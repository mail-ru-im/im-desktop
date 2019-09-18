#include "stdafx.h"

#include "../sys.h"

#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CFURL.h>
#include <sys/sysctl.h>

namespace System
{
    void launchApplication(const QString& _path)
    {
        NSString * pathString = CFBridgingRelease(_path.toCFString());
        NSDictionary *conf = [NSDictionary dictionaryWithObject:[NSArray array] forKey:NSWorkspaceLaunchConfigurationArguments];
        [[NSWorkspace sharedWorkspace] launchApplicationAtURL:[NSURL fileURLWithPath:pathString] options:NSWorkspaceLaunchAsync | NSWorkspaceLaunchNewInstance configuration:conf error:0];
    }
}
