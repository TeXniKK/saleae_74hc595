#include "HC595AnalyzerSettings.h"
#include <AnalyzerHelpers.h>

HC595AnalyzerSettings::HC595AnalyzerSettings()
    : mDataChannel( UNDEFINED_CHANNEL ),
      mShiftChannel( UNDEFINED_CHANNEL ),
      mLatchChannel( UNDEFINED_CHANNEL ),
      mResetChannel( UNDEFINED_CHANNEL ),
      mEnableChannel( UNDEFINED_CHANNEL ),
      mBitsPerWord( 8 ),
      mShiftClockEdge( 0 ),
      mLatchClockEdge( 0 ),
      mBitOrder( 0 )
{
    mDataChannelInterface.SetTitleAndTooltip( "SER/DS (Serial Data)", "74HC595 pin 14 - serial data input (DS / SER)" );
    mDataChannelInterface.SetChannel( mDataChannel );

    mShiftChannelInterface.SetTitleAndTooltip( "SRCLK/SHCP (Shift Clock)", "74HC595 pin 11 - shift register clock (SHCP / SRCLK). Data is sampled on this edge." );
    mShiftChannelInterface.SetChannel( mShiftChannel );

    mLatchChannelInterface.SetTitleAndTooltip( "RCLK/STCP (Latch Clock)", "74HC595 pin 12 - storage register clock (STCP / RCLK). The shifted word is latched to the outputs on this edge." );
    mLatchChannelInterface.SetChannel( mLatchChannel );

    mResetChannelInterface.SetTitleAndTooltip( "SRCLR/MR (Reset, optional)", "74HC595 pin 10 - master reset (MR / SRCLR), active low. Leave as 'None' if unused." );
    mResetChannelInterface.SetChannel( mResetChannel );
    mResetChannelInterface.SetSelectionOfNoneIsAllowed( true );

    mEnableChannelInterface.SetTitleAndTooltip( "OE (Output Enable, optional)", "74HC595 pin 13 - output enable (OE), active low. Leave as 'None' if unused." );
    mEnableChannelInterface.SetChannel( mEnableChannel );
    mEnableChannelInterface.SetSelectionOfNoneIsAllowed( true );

    mBitsPerWordInterface.SetTitleAndTooltip( "Bits per Word", "Number of bits latched per output word. Use 8 for a single device, or a multiple of 8 for daisy-chained devices (max 32)." );
    mBitsPerWordInterface.SetMax( 32 );
    mBitsPerWordInterface.SetMin( 1 );
    mBitsPerWordInterface.SetInteger( mBitsPerWord );

    mShiftClockEdgeInterface.SetTitleAndTooltip( "Shift Clock Active Edge", "Edge of SHCP on which the serial data is sampled. The 74HC595 shifts on the rising edge." );
    mShiftClockEdgeInterface.AddNumber( 0, "Rising edge (74HC595 default)", "Sample DS on the rising edge of SHCP." );
    mShiftClockEdgeInterface.AddNumber( 1, "Falling edge", "Sample DS on the falling edge of SHCP." );
    mShiftClockEdgeInterface.SetNumber( mShiftClockEdge );

    mLatchClockEdgeInterface.SetTitleAndTooltip( "Latch Clock Active Edge", "Edge of STCP that latches the shift register to the outputs. The 74HC595 latches on the rising edge." );
    mLatchClockEdgeInterface.AddNumber( 0, "Rising edge (74HC595 default)", "Latch on the rising edge of STCP." );
    mLatchClockEdgeInterface.AddNumber( 1, "Falling edge", "Latch on the falling edge of STCP." );
    mLatchClockEdgeInterface.SetNumber( mLatchClockEdge );

    mBitOrderInterface.SetTitleAndTooltip( "Bit Order", "How the serial bit stream maps to the latched value. The 74HC595 is normally fed MSB first." );
    mBitOrderInterface.AddNumber( 0, "MSB first (first bit shifted in is MSB)", "The first bit clocked in becomes the most significant bit of the word." );
    mBitOrderInterface.AddNumber( 1, "LSB first (first bit shifted in is LSB)", "The first bit clocked in becomes the least significant bit of the word." );
    mBitOrderInterface.SetNumber( mBitOrder );

    AddInterface( &mDataChannelInterface );
    AddInterface( &mShiftChannelInterface );
    AddInterface( &mLatchChannelInterface );
    AddInterface( &mResetChannelInterface );
    AddInterface( &mEnableChannelInterface );
    AddInterface( &mBitsPerWordInterface );
    AddInterface( &mShiftClockEdgeInterface );
    AddInterface( &mLatchClockEdgeInterface );
    AddInterface( &mBitOrderInterface );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mDataChannel, "DS", false );
    AddChannel( mShiftChannel, "SHCP", false );
    AddChannel( mLatchChannel, "STCP", false );
    AddChannel( mResetChannel, "MR", false );
    AddChannel( mEnableChannel, "OE", false );
}

HC595AnalyzerSettings::~HC595AnalyzerSettings()
{
}

bool HC595AnalyzerSettings::SetSettingsFromInterfaces()
{
    Channel data = mDataChannelInterface.GetChannel();
    Channel shift = mShiftChannelInterface.GetChannel();
    Channel latch = mLatchChannelInterface.GetChannel();
    Channel reset = mResetChannelInterface.GetChannel();
    Channel enable = mEnableChannelInterface.GetChannel();

    if( data == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select a channel for DS (serial data)." );
        return false;
    }
    if( shift == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select a channel for SHCP (shift clock)." );
        return false;
    }
    if( latch == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select a channel for STCP (latch clock)." );
        return false;
    }

    // Ensure no channel is assigned to more than one role.
    Channel assigned[ 5 ];
    U32 count = 0;
    assigned[ count++ ] = data;
    assigned[ count++ ] = shift;
    assigned[ count++ ] = latch;
    if( reset != UNDEFINED_CHANNEL )
        assigned[ count++ ] = reset;
    if( enable != UNDEFINED_CHANNEL )
        assigned[ count++ ] = enable;

    for( U32 i = 0; i < count; i++ )
        for( U32 j = i + 1; j < count; j++ )
            if( assigned[ i ] == assigned[ j ] )
            {
                SetErrorText( "Each role (DS, SHCP, STCP, MR, OE) must use a different channel." );
                return false;
            }

    mDataChannel = data;
    mShiftChannel = shift;
    mLatchChannel = latch;
    mResetChannel = reset;
    mEnableChannel = enable;

    mBitsPerWord = mBitsPerWordInterface.GetInteger();
    mShiftClockEdge = ( U32 )mShiftClockEdgeInterface.GetNumber();
    mLatchClockEdge = ( U32 )mLatchClockEdgeInterface.GetNumber();
    mBitOrder = ( U32 )mBitOrderInterface.GetNumber();

    ClearChannels();
    AddChannel( mDataChannel, "DS", true );
    AddChannel( mShiftChannel, "SHCP", true );
    AddChannel( mLatchChannel, "STCP", true );
    AddChannel( mResetChannel, "MR", mResetChannel != UNDEFINED_CHANNEL );
    AddChannel( mEnableChannel, "OE", mEnableChannel != UNDEFINED_CHANNEL );

    return true;
}

void HC595AnalyzerSettings::UpdateInterfacesFromSettings()
{
    mDataChannelInterface.SetChannel( mDataChannel );
    mShiftChannelInterface.SetChannel( mShiftChannel );
    mLatchChannelInterface.SetChannel( mLatchChannel );
    mResetChannelInterface.SetChannel( mResetChannel );
    mEnableChannelInterface.SetChannel( mEnableChannel );
    mBitsPerWordInterface.SetInteger( mBitsPerWord );
    mShiftClockEdgeInterface.SetNumber( mShiftClockEdge );
    mLatchClockEdgeInterface.SetNumber( mLatchClockEdge );
    mBitOrderInterface.SetNumber( mBitOrder );
}

void HC595AnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mDataChannel;
    text_archive >> mShiftChannel;
    text_archive >> mLatchChannel;
    text_archive >> mResetChannel;
    text_archive >> mEnableChannel;
    text_archive >> mBitsPerWord;
    text_archive >> mShiftClockEdge;
    text_archive >> mLatchClockEdge;
    text_archive >> mBitOrder;

    ClearChannels();
    AddChannel( mDataChannel, "DS", mDataChannel != UNDEFINED_CHANNEL );
    AddChannel( mShiftChannel, "SHCP", mShiftChannel != UNDEFINED_CHANNEL );
    AddChannel( mLatchChannel, "STCP", mLatchChannel != UNDEFINED_CHANNEL );
    AddChannel( mResetChannel, "MR", mResetChannel != UNDEFINED_CHANNEL );
    AddChannel( mEnableChannel, "OE", mEnableChannel != UNDEFINED_CHANNEL );

    UpdateInterfacesFromSettings();
}

const char* HC595AnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mDataChannel;
    text_archive << mShiftChannel;
    text_archive << mLatchChannel;
    text_archive << mResetChannel;
    text_archive << mEnableChannel;
    text_archive << mBitsPerWord;
    text_archive << mShiftClockEdge;
    text_archive << mLatchClockEdge;
    text_archive << mBitOrder;

    return SetReturnString( text_archive.GetString() );
}
