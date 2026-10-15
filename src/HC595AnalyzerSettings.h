#ifndef HC595_ANALYZER_SETTINGS
#define HC595_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class HC595AnalyzerSettings : public AnalyzerSettings
{
public:
    HC595AnalyzerSettings();
    virtual ~HC595AnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    // Channels
    Channel mDataChannel;   // DS / SER
    Channel mShiftChannel;  // SHCP / SRCLK
    Channel mLatchChannel;  // STCP / RCLK
    Channel mResetChannel;  // MR / SRCLR (active low), optional
    Channel mEnableChannel; // OE (active low), optional

    // Behaviour
    U32 mBitsPerWord;      // number of bits latched per word (8..32 for daisy chains)
    U32 mShiftClockEdge;   // 0 = rising (default for 74HC595), 1 = falling
    U32 mLatchClockEdge;   // 0 = rising (default for 74HC595), 1 = falling
    U32 mBitOrder;         // 0 = first bit shifted in is MSB, 1 = first bit shifted in is LSB

protected:
    AnalyzerSettingInterfaceChannel mDataChannelInterface;
    AnalyzerSettingInterfaceChannel mShiftChannelInterface;
    AnalyzerSettingInterfaceChannel mLatchChannelInterface;
    AnalyzerSettingInterfaceChannel mResetChannelInterface;
    AnalyzerSettingInterfaceChannel mEnableChannelInterface;

    AnalyzerSettingInterfaceInteger mBitsPerWordInterface;
    AnalyzerSettingInterfaceNumberList mShiftClockEdgeInterface;
    AnalyzerSettingInterfaceNumberList mLatchClockEdgeInterface;
    AnalyzerSettingInterfaceNumberList mBitOrderInterface;
};

#endif // HC595_ANALYZER_SETTINGS
