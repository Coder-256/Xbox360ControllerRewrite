
#include <IOKit/usb/IOUSBHostInterface.h>
#include <IOKit/usb/IOUSBHostPipe.h>
#include <IOKit/hid/IOHIDDevice.h>

class Xbox360ControllerRewrite : public IOHIDDevice
{
    OSDeclareDefaultStructors(Xbox360ControllerRewrite)
    
protected:
    IOUSBHostPipe* _inPipe;
    IOUSBHostPipe* _outPipe;
    
protected:
    void releaseAll(void);
    
public:
    virtual bool handleStart(IOService* provider) override;
    virtual void handleStop(IOService* provider) override;
    virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
};
