#ifndef HC595_ANALYZER_RESULTS
#define HC595_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class HC595Analyzer;
class HC595AnalyzerSettings;

// Frame flag bits (stored in Frame.mFlags)
#define HC595_FLAG_RESET ( 1 << 0 ) // this word was terminated / affected by a master reset

class HC595AnalyzerResults : public AnalyzerResults
{
public:
    HC595AnalyzerResults( HC595Analyzer* analyzer, HC595AnalyzerSettings* settings );
    virtual ~HC595AnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText( U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: // vars
    HC595AnalyzerSettings* mSettings;
    HC595Analyzer* mAnalyzer;
};

#endif // HC595_ANALYZER_RESULTS
