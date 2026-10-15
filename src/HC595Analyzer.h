#ifndef HC595_ANALYZER_H
#define HC595_ANALYZER_H

#include <Analyzer.h>
#include "HC595AnalyzerSettings.h"
#include "HC595AnalyzerResults.h"
#include "HC595SimulationDataGenerator.h"
#include <memory>

class ANALYZER_EXPORT HC595Analyzer : public Analyzer2
{
public:
    HC595Analyzer();
    virtual ~HC595Analyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

protected:
    // Advances every subscribed channel so they stay aligned to the given sample.
    void AdvanceAllTo( U64 sample );

protected: // vars
    HC595AnalyzerSettings mSettings;
    std::unique_ptr<HC595AnalyzerResults> mResults;

    AnalyzerChannelData* mData;   // DS / SER   (serial data input)
    AnalyzerChannelData* mShift;  // SHCP / SRCLK (shift register clock)
    AnalyzerChannelData* mLatch;  // STCP / RCLK  (storage register clock)
    AnalyzerChannelData* mReset;  // MR / SRCLR   (master reset, active low) - optional
    AnalyzerChannelData* mEnable; // OE           (output enable, active low) - optional

    HC595SimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitialized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // HC595_ANALYZER_H
