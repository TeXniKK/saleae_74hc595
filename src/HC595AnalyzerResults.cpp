#include "HC595AnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "HC595Analyzer.h"
#include "HC595AnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <string>

HC595AnalyzerResults::HC595AnalyzerResults( HC595Analyzer* analyzer, HC595AnalyzerSettings* settings )
    : AnalyzerResults(),
      mSettings( settings ),
      mAnalyzer( analyzer )
{
}

HC595AnalyzerResults::~HC595AnalyzerResults()
{
}

void HC595AnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
    ClearResultStrings();
    Frame frame = GetFrame( frame_index );

    U32 width = mSettings->mBitsPerWord;
    if( width > 32 )
        width = 32;

    if( ( frame.mFlags & HC595_FLAG_RESET ) != 0 && frame.mData2 != width )
    {
        // A partial word cleared by a master reset.
        AddResultString( "RESET" );
        AddResultString( "Shift register reset" );
        return;
    }

    char number_str[ 128 ];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, width, number_str, sizeof( number_str ) );

    // Short to long variants so Logic can pick what fits the available space.
    AddResultString( number_str );
    AddResultString( "Q: ", number_str );

    char bits_str[ 16 ];
    AnalyzerHelpers::GetNumberString( ( U64 )frame.mData2, Decimal, 32, bits_str, sizeof( bits_str ) );
    AddResultString( "Latched ", number_str, " (", bits_str, " bits shifted)" );
}

void HC595AnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
    std::ofstream file_stream( file, std::ios::out );

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    U32 width = mSettings->mBitsPerWord;
    if( width > 32 )
        width = 32;

    file_stream << "Time [s],Value,Bits Shifted,Reset" << std::endl;

    U64 num_frames = GetNumFrames();
    for( U64 i = 0; i < num_frames; i++ )
    {
        Frame frame = GetFrame( i );

        char time_str[ 128 ];
        AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof( time_str ) );

        char number_str[ 128 ];
        AnalyzerHelpers::GetNumberString( frame.mData1, display_base, width, number_str, sizeof( number_str ) );

        bool is_reset = ( frame.mFlags & HC595_FLAG_RESET ) != 0;

        file_stream << time_str << "," << number_str << "," << ( U64 )frame.mData2 << ","
                    << ( is_reset ? "yes" : "no" ) << std::endl;

        if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
        {
            file_stream.close();
            return;
        }
    }

    file_stream.close();
}

void HC595AnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
    Frame frame = GetFrame( frame_index );
    ClearTabularText();

    U32 width = mSettings->mBitsPerWord;
    if( width > 32 )
        width = 32;

    if( ( frame.mFlags & HC595_FLAG_RESET ) != 0 && frame.mData2 != width )
    {
        AddTabularText( "RESET" );
        return;
    }

    char number_str[ 128 ];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, width, number_str, sizeof( number_str ) );
    AddTabularText( number_str );
#endif
}

void HC595AnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
    // not supported
}

void HC595AnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
    // not supported
}
