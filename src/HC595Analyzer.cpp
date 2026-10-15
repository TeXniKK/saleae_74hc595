#include "HC595Analyzer.h"
#include "HC595AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

HC595Analyzer::HC595Analyzer()
    : Analyzer2(),
      mSettings(),
      mData( NULL ),
      mShift( NULL ),
      mLatch( NULL ),
      mReset( NULL ),
      mEnable( NULL ),
      mSimulationInitialized( false )
{
    SetAnalyzerSettings( &mSettings );
}

HC595Analyzer::~HC595Analyzer()
{
    KillThread();
}

void HC595Analyzer::SetupResults()
{
    mResults.reset( new HC595AnalyzerResults( this, &mSettings ) );
    SetAnalyzerResults( mResults.get() );
    // Decoded words appear on the latch (STCP) channel, since that is where a word becomes visible on the outputs.
    mResults->AddChannelBubblesWillAppearOn( mSettings.mLatchChannel );
}

void HC595Analyzer::AdvanceAllTo( U64 sample )
{
    // AdvanceToAbsPosition is a no-op if the channel is already at/after the sample.
    if( mData != NULL )
        mData->AdvanceToAbsPosition( sample );
    if( mShift != NULL )
        mShift->AdvanceToAbsPosition( sample );
    if( mLatch != NULL )
        mLatch->AdvanceToAbsPosition( sample );
    if( mReset != NULL )
        mReset->AdvanceToAbsPosition( sample );
    if( mEnable != NULL )
        mEnable->AdvanceToAbsPosition( sample );
}

void HC595Analyzer::WorkerThread()
{
    mData = GetAnalyzerChannelData( mSettings.mDataChannel );
    mShift = GetAnalyzerChannelData( mSettings.mShiftChannel );
    mLatch = GetAnalyzerChannelData( mSettings.mLatchChannel );
    mReset = ( mSettings.mResetChannel != UNDEFINED_CHANNEL ) ? GetAnalyzerChannelData( mSettings.mResetChannel ) : NULL;
    mEnable = ( mSettings.mEnableChannel != UNDEFINED_CHANNEL ) ? GetAnalyzerChannelData( mSettings.mEnableChannel ) : NULL;

    const BitState shift_active = ( mSettings.mShiftClockEdge == 0 ) ? BIT_HIGH : BIT_LOW; // level AFTER the active edge
    const BitState latch_active = ( mSettings.mLatchClockEdge == 0 ) ? BIT_HIGH : BIT_LOW;
    const U32 bits_per_word = mSettings.mBitsPerWord;
    const bool msb_first = ( mSettings.mBitOrder == 0 );

    // Internal shift register. Index 0 is the first bit that was clocked in for the current word.
    // We keep it as a 64-bit accumulator and assemble the value when the word is complete / latched.
    U64 shift_reg = 0;      // bits already clocked in, in arrival order (bit 0 = first in)
    U32 shift_count = 0;    // number of bits currently in the register
    U64 word_start_sample = 0; // sample of the first shift edge of the current word
    bool have_word_start = false;
    bool reset_pending = false; // a master reset occurred while filling the current word

    for( ;; )
    {
        // Find the next event: either a shift-clock active edge or a latch-clock active edge,
        // whichever comes first. We also watch the reset line for its active (low) edge.
        U64 next_shift = mShift->GetSampleOfNextEdge();
        U64 next_latch = mLatch->GetSampleOfNextEdge();
        U64 next_reset = ( mReset != NULL ) ? mReset->GetSampleOfNextEdge() : 0xFFFFFFFFFFFFFFFFULL;

        // Pick the earliest upcoming edge among the channels we track.
        U64 target = next_shift;
        if( next_latch < target )
            target = next_latch;
        if( mReset != NULL && next_reset < target )
            target = next_reset;

        // Advance the clock/control channels to that edge, and the data channel to the same sample.
        if( next_shift == target )
            mShift->AdvanceToNextEdge();
        if( next_latch == target )
            mLatch->AdvanceToNextEdge();
        if( mReset != NULL && next_reset == target )
            mReset->AdvanceToNextEdge();

        AdvanceAllTo( target );

        // --- Handle master reset (active low): SRCLR cleared the shift register. ---
        if( mReset != NULL && next_reset == target && mReset->GetBitState() == BIT_LOW )
        {
            if( shift_count > 0 )
            {
                // Mark the partially-filled word as discarded by a reset.
                Frame frame;
                frame.mData1 = 0;
                frame.mData2 = shift_count;
                frame.mFlags = HC595_FLAG_RESET;
                frame.mStartingSampleInclusive = have_word_start ? word_start_sample : target;
                frame.mEndingSampleInclusive = target;
                mResults->AddFrame( frame );
                mResults->CommitResults();
            }
            mResults->AddMarker( target, AnalyzerResults::X, mSettings.mResetChannel );
            shift_reg = 0;
            shift_count = 0;
            have_word_start = false;
            reset_pending = false;
            ReportProgress( target );
            continue;
        }

        // --- Handle a shift-clock active edge: sample DS and shift it in. ---
        if( next_shift == target && mShift->GetBitState() == shift_active )
        {
            BitState ds = mData->GetBitState();

            if( !have_word_start )
            {
                word_start_sample = target;
                have_word_start = true;
            }

            if( shift_count < 64 )
            {
                if( ds == BIT_HIGH )
                    shift_reg |= ( U64( 1 ) << shift_count );
                // bit position 'shift_count' holds this (shift_count-th) arriving bit
            }
            shift_count++;

            mResults->AddMarker( target,
                                 ds == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero,
                                 mSettings.mDataChannel );
            ReportProgress( target );
        }

        // --- Handle a latch-clock active edge: emit the word currently in the register. ---
        if( next_latch == target && mLatch->GetBitState() == latch_active )
        {
            // Assemble the latched value from the arrival-ordered bits.
            //
            // On a 74HC595 bits propagate QA -> QB -> ... so the FIRST bit clocked in
            // travels furthest and ends up at the last stage (QH for 8 bits).
            // With the common "MSB first" wiring the first bit sent is therefore the MSB.
            //
            // shift_reg stores bits in arrival order: bit position k == the (k)-th bit
            // that was clocked in (k = 0 is the first one in for this word).
            //
            // We present the last 'width' arrived bits (the ones occupying the output stages).
            U32 width = bits_per_word;
            if( width > 32 )
                width = 32;

            U64 value = 0;
            U32 take = ( shift_count < width ) ? shift_count : width;
            // Offset so that if more bits than 'width' were clocked, only the most recent
            // 'width' bits are shown (older ones have fallen off the end of the chain).
            U32 base = shift_count - take; // arrival index of the oldest bit still shown

            for( U32 i = 0; i < take; i++ )
            {
                U32 arrival_index = base + i;              // 0 = oldest shown, increasing = newer
                U64 bit = ( shift_reg >> arrival_index ) & 0x1;
                U32 out_pos;
                if( msb_first )
                    out_pos = ( width - 1 ) - i; // oldest/first bit -> MSB
                else
                    out_pos = i;                 // oldest/first bit -> LSB
                value |= ( bit << out_pos );
            }

            Frame frame;
            frame.mData1 = value;
            frame.mData2 = shift_count; // how many bits had been clocked in before this latch
            frame.mFlags = reset_pending ? HC595_FLAG_RESET : 0;
            frame.mStartingSampleInclusive = have_word_start ? word_start_sample : target;
            frame.mEndingSampleInclusive = target;
            mResults->AddFrame( frame );
            mResults->CommitResults();

            mResults->AddMarker( target, AnalyzerResults::UpArrow, mSettings.mLatchChannel );

            // A real device does NOT clear the shift register on latch; the bits remain and
            // continue to shift. But for decoding discrete words we start a fresh word count
            // so each latch shows the word that was just presented.
            shift_reg = 0;
            shift_count = 0;
            have_word_start = false;
            reset_pending = false;
            ReportProgress( target );
        }
    }
}

bool HC595Analyzer::NeedsRerun()
{
    return false;
}

U32 HC595Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitialized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
        mSimulationInitialized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 HC595Analyzer::GetMinimumSampleRateHz()
{
    // The analyzer has no fixed bit rate; recommend a modest minimum. Logic enforces its own capture rate.
    return 1000000;
}

const char* HC595Analyzer::GetAnalyzerName() const
{
    return "74HC595 Shift Register";
}

const char* GetAnalyzerName()
{
    return "74HC595 Shift Register";
}

Analyzer* CreateAnalyzer()
{
    return new HC595Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}
