
#include "Xbox360ControllerRewrite.hpp"

namespace HID_360 {
#include "xbox360hid.h"
}

OSDefineMetaClassAndStructors(Xbox360ControllerRewrite, IOHIDDevice)

#define super IOHIDDevice

bool Xbox360ControllerRewrite::handleStart(IOService* provider)
{
    IOLog("360Controller - handleStart()\n");
    
    const ConfigurationDescriptor* configurationDescriptor = nullptr;
    const EndpointDescriptor* pipeCandidate = nullptr;
    const InterfaceDescriptor* interfaceDescriptor = nullptr;
    IOUSBHostInterface* interface = nullptr;
    
    
    if (!super::handleStart(provider))
    {
        IOLog("360Controller - handleStart() - Failed when calling super's handleStart()\n");
        goto cleanup;
    }
    
    interface = OSDynamicCast(IOUSBHostInterface, provider);
    if (interface == nullptr)
    {
        IOLog("360Controller - handleStart() - Interface could not be cast to an IOUSBHostInterface\n");
        goto cleanup;
    }
    interface->open(this);
    IOLog("360Controller - DEBUG - Matched interface of class: %d\n", interface->getInterfaceDescriptor()->bInterfaceClass); // TODO(Drew): Delete me
    IOLog("360Controller - DEBUG - Matched interface of subclass: %d\n", interface->getInterfaceDescriptor()->bInterfaceSubClass); // TODO(Drew): Delete me
    IOLog("360Controller - DEBUG - Matched interface of protocol: %d\n", interface->getInterfaceDescriptor()->bInterfaceProtocol); // TODO(Drew): Delete me
    IOLog("360Controller - DEBUG - Matched interface of number: %d\n", interface->getInterfaceDescriptor()->bInterfaceNumber); // TODO(Drew): Delete me
    
    configurationDescriptor = interface->getConfigurationDescriptor();
    if (configurationDescriptor == nullptr)
    {
        IOLog("360Controller - handleStart() - Configuration descriptor was null\n");
        goto cleanup;
    }
    
    interfaceDescriptor = interface->getInterfaceDescriptor();
    if (interfaceDescriptor == nullptr)
    {
        IOLog("360Controller - handleStart() - Interface descriptor was null\n");
        goto cleanup;
    }
    
    while ((pipeCandidate = StandardUSB::getNextEndpointDescriptor(configurationDescriptor, interfaceDescriptor, pipeCandidate)))
    {
        UInt8 pipeDirection = StandardUSB::getEndpointDirection(pipeCandidate);
        UInt8 pipeType = StandardUSB::getEndpointType(pipeCandidate);
        UInt8 pipeAddress = StandardUSB::getEndpointAddress(pipeCandidate);
        
        if (pipeDirection == kEndpointDirectionIn && pipeType == kEndpointTypeInterrupt)
        {
            _inPipe = interface->copyPipe(pipeAddress);
        }
        else if (pipeDirection == kEndpointDirectionOut && pipeType == kEndpointTypeInterrupt)
        {
            _outPipe = interface->copyPipe(pipeAddress);
        }
    }
    
    if (_inPipe == nullptr)
    {
        IOLog("360Controller - handleStart() - Input pipe is null\n");
        goto cleanup;
    }
    _inPipe->retain();
    
    if (_outPipe == nullptr)
    {
        IOLog("360Controller - handleStart() - Output pipe is null\n");
        goto cleanup;
    }
    _outPipe->retain();
    
    return super::handleStart(provider);
    
cleanup:
    releaseAll();
    return false;
}

void Xbox360ControllerRewrite::handleStop(IOService* provider)
{
    IOLog("360Controller - handleStop()\n");
    
    releaseAll();
    
    return IOHIDDevice::handleStop(provider);
}

IOReturn Xbox360ControllerRewrite::newReportDescriptor(IOMemoryDescriptor** descriptor) const
{
    IOLog("360Controller - newReportDescriptor\n");

    IOMemoryDescriptor* buffer = nullptr;
    IOByteCount written = 0;
    
    if (descriptor == nullptr)
    {
        IOLog("360Controller - newReportDescriptor() - Was passed a null pointer from macOS kernel\n");
        return kIOReturnBadArgument;
    }
    
    buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, HID_360::reportDescriptorSize);
    if (buffer == nullptr)
    {
        IOLog("360Controller - newReportDescriptor() - No memory to initialize buffer\n");
        return kIOReturnNoMemory;
    }
    
    written = buffer->writeBytes(0, HID_360::reportDescriptor, HID_360::reportDescriptorSize);
    if (written != HID_360::reportDescriptorSize)
    {
        IOLog("360Controller - newReportDescriptor() - Report descriptor size was not the same as expected\n");
        buffer->release();
        return kIOReturnNoSpace;
    }
    
    *descriptor = buffer;
    return kIOReturnSuccess;
}

void Xbox360ControllerRewrite::releaseAll(void)
{
    if (_inPipe != nullptr)
    {
        _inPipe->abort();
        _inPipe->release();
        _inPipe = nullptr;
    }
    
    if (_outPipe != nullptr)
    {
        _outPipe->abort();
        _outPipe->release();
        _outPipe = nullptr;
    }
}
