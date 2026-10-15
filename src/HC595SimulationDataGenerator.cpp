#include "HC595SimulationDataGenerator.h"
#include "HC595AnalyzerSettings.h"

#include <AnalyzerHelpers.h>

HC595SimulationDataGenerator::HC595SimulationDataGenerator()
    : mSettings( NULL ),
      mSimulationSampleRateHz( 0 ),
      mValue( 0 ),
      mDataSim( NULL ),
      mShiftSim( NULL ),
      mLatchSim( NULL )
{
}

HC595SimulationDataGenerator::~HC595SimulationDataGenerator()
{
}

void HC595SimulationDataGenerator::Initialize( U32 simulation_sample_rate, HC595AnalyzerSettings* settings )
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    // Idle levels: DS low, clocks low.
    mDataSim = mSimulationChannels.Add( mSettings->mDataChannel, simulation_sample_rate, BIT_LOW );
    mShiftSim = mSimulationChannels.Add( mSettings->mShiftChannel, simulation_sample_rate, BIT_LOW );
    mLatchSim = mSimulationChannels.Add( mSettings->mLatchChannel, simulation_sample_rate, BIT_LOW );

    // Idle for a moment before the first transfer.
    mSimulationChannels.AdvanceAll( simulation_sample_rate / 10 );
}

U32 HC595SimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate,
                                                          SimulationChannelDescriptor** simulation_channel )
{
    U64 adjusted_largest_sample_requested =
        AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

    while( mShiftSim->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
    {
        ShiftOutWord( mValue );
        mValue = ( mValue + 1 ) & 0xFFFFFFFF;
    }

    *simulation_channel = mSimulationChannels.GetArray();
    return mSimulationChannels.GetCount();
}

void HC595SimulationDataGenerator::ShiftOutWord( U32 value )
{
    U32 width = mSettings->mBitsPerWord;
    if( width > 32 )
        width = 32;

    const bool msb_first = ( mSettings->mBitOrder == 0 );
    const BitState shift_active = ( mSettings->mShiftClockEdge == 0 ) ? BIT_HIGH : BIT_LOW;
    const BitState latch_active = ( mSettings->mLatchClockEdge == 0 ) ? BIT_HIGH : BIT_LOW;

    // One "clock period" in simulation samples. Keep it comfortably long.
    U32 samples_per_half = mSimulationSampleRateHz / 2000000; // ~1 MHz clock
    if( samples_per_half < 2 )
        samples_per_half = 2;

    // Make sure clocks start at their inactive level.
    mShiftSim->TransitionIfNeeded( ( shift_active == BIT_HIGH ) ? BIT_LOW : BIT_HIGH );
    mLatchSim->TransitionIfNeeded( ( latch_active == BIT_HIGH ) ? BIT_LOW : BIT_HIGH );

    for( U32 i = 0; i < width; i++ )
    {
        // Select which source bit goes out first. First-out bit:
        //   MSB first -> bit (width-1), LSB first -> bit 0.
        U32 src_bit = msb_first ? ( width - 1 - i ) : i;
        BitState ds = ( ( value >> src_bit ) & 0x1 ) ? BIT_HIGH : BIT_LOW;

        // Present data while the shift clock is at its inactive level.
        mDataSim->TransitionIfNeeded( ds );
        mSimulationChannels.AdvanceAll( samples_per_half );

        // Active edge of the shift clock samples DS.
        mShiftSim->Transition();
        mSimulationChannels.AdvanceAll( samples_per_half );

        // Return the shift clock to the inactive level.
        mShiftSim->Transition();
    }

    // Small gap before latching.
    mSimulationChannels.AdvanceAll( samples_per_half * 2 );

    // Pulse the latch clock: active edge, hold, return.
    mLatchSim->Transition();
    mSimulationChannels.AdvanceAll( samples_per_half );
    mLatchSim->Transition();

    // Idle gap between words.
    mDataSim->TransitionIfNeeded( BIT_LOW );
    mSimulationChannels.AdvanceAll( samples_per_half * 8 );
}
