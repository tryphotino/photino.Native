#ifdef __APPLE__
#import "Photino.Dialog.h"

#if defined(VSTGUI_USE_OBJC_UTTYPE)
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#else
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

NSString* errorBase64 = @"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";
NSString* infoBase64 = @"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";
NSString* questionBase64 = @"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";
NSString* warningBase64 = @"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";

NSImage* getIcon(NSString* base64) {
  NSData *imageData = [[NSData alloc] initWithBase64EncodedString:base64 options:0];
  NSImage *image = [[NSImage alloc] initWithData:imageData];
  return image;
}

PhotinoDialog::PhotinoDialog() {
  _errorIcon = getIcon(errorBase64);
  _infoIcon = getIcon(infoBase64);
  _questionIcon = getIcon(questionBase64);
  _warningIcon = getIcon(warningBase64);
}

PhotinoDialog::~PhotinoDialog() {
}

AutoString* PhotinoDialog::ShowOpenFile(AutoString title, AutoString defaultPath, bool multiSelect, AutoString* filters, int filterCount, int* resultCount) {
  NSOpenPanel* openDlg = [NSOpenPanel openPanel];

  [openDlg setTitle:[NSString stringWithUTF8String:title]];
  [openDlg setCanChooseFiles:YES];
  [openDlg setCanChooseDirectories:NO];
  [openDlg setAllowsMultipleSelection:multiSelect];
  [openDlg setPrompt:[NSString stringWithUTF8String:"Open"]];
  [openDlg setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath]]];

  if (filterCount > 0) {
    NSMutableArray* fileTypes = [[[NSMutableArray alloc] init] autorelease];
    for (int i = 0; i < filterCount; i++) {
      [fileTypes addObject:[NSString stringWithUTF8String:filters[i]]];
    }

#ifdef VSTGUI_USE_OBJC_UTTYPE
			[openDlg setAllowedContentTypes:fileTypes];
#else
			[openDlg setAllowedFileTypes:fileTypes];
#endif
  }

  if ([openDlg runModal] == NSModalResponseOK) {
    NSArray* files = [openDlg URLs];
    *resultCount = (int)[files count];
    char** result = (char**)malloc(*resultCount * sizeof(char*));
    for (int i = 0; i < *resultCount; i++) {
      result[i] = strdup([[[files objectAtIndex:i] path] UTF8String]);
    }
    return result;
  }

  return nullptr;
}

AutoString* PhotinoDialog::ShowOpenFolder(AutoString title, AutoString defaultPath, bool multiSelect, int* resultCount) {
  NSOpenPanel* openDlg = [NSOpenPanel openPanel];

  [openDlg setTitle:[NSString stringWithUTF8String:title]];
  [openDlg setCanChooseFiles:NO];
  [openDlg setCanChooseDirectories:YES];
  [openDlg setCanCreateDirectories:YES];
  [openDlg setAllowsMultipleSelection:multiSelect];
  [openDlg setPrompt:[NSString stringWithUTF8String:"Open"]];
  [openDlg setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath]]];

  if ([openDlg runModal] == NSModalResponseOK) {
    NSArray* files = [openDlg URLs];
    *resultCount = (int)[files count];
    char** result = (char**)malloc(*resultCount * sizeof(char*));
    for (int i = 0; i < *resultCount; i++) {
      result[i] = strdup([[[files objectAtIndex:i] path] UTF8String]);
    }
    return result;
  }

  return nullptr;
}

AutoString PhotinoDialog::ShowSaveFile(AutoString title, AutoString defaultPath, AutoString* filters, int filterCount) {
  NSSavePanel* saveDlg = [NSSavePanel savePanel];

  [saveDlg setTitle:[NSString stringWithUTF8String:title]];
  [saveDlg setPrompt:[NSString stringWithUTF8String:"Save"]];
  [saveDlg setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath]]];
  [saveDlg setAllowsOtherFileTypes:NO];
  [saveDlg setCanSelectHiddenExtension:YES];

  if (filterCount > 0) {
    NSMutableArray* fileTypes = [[[NSMutableArray alloc] init] autorelease];
    for (int i = 0; i < filterCount; i++) {
      [fileTypes addObject:[NSString stringWithUTF8String:filters[i]]];
    }

#ifdef VSTGUI_USE_OBJC_UTTYPE
			[saveDlg setAllowedContentTypes:fileTypes];
#else
			[saveDlg setAllowedFileTypes:fileTypes];
#endif
  }

  if ([saveDlg runModal] == NSModalResponseOK) {
    return strdup([[saveDlg URL].path UTF8String]);
  }

  return nullptr;
}

DialogResult PhotinoDialog::ShowMessage(AutoString title, AutoString text, DialogButtons buttons, DialogIcon icon) {
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:[NSString stringWithUTF8String:title]];
  [alert setInformativeText:[NSString stringWithUTF8String:text]];
  
  switch (buttons) {
    case DialogButtons::Ok:
      [alert addButtonWithTitle:@"OK"];
      break;
    case DialogButtons::OkCancel:
      [alert addButtonWithTitle:@"OK"];
      [alert addButtonWithTitle:@"Cancel"];
      break;
    case DialogButtons::YesNo:
      [alert addButtonWithTitle:@"Yes"];
      [alert addButtonWithTitle:@"No"];
      break;
    case DialogButtons::YesNoCancel:
      [alert addButtonWithTitle:@"Yes"];
      [alert addButtonWithTitle:@"No"];
      [alert addButtonWithTitle:@"Cancel"];
      break;
    case DialogButtons::RetryCancel:
      [alert addButtonWithTitle:@"Retry"];
      [alert addButtonWithTitle:@"Cancel"];
      break;
    case DialogButtons::AbortRetryIgnore:
      [alert addButtonWithTitle:@"Abort"];
      [alert addButtonWithTitle:@"Retry"];
      [alert addButtonWithTitle:@"Ignore"];
      break;
  }

  switch (icon) {
    case DialogIcon::Error:
      [alert setIcon:_errorIcon];
      break;
    case DialogIcon::Warning:
      [alert setIcon:_warningIcon];
      break;
    case DialogIcon::Info:
      [alert setIcon:_infoIcon];
      break;
    case DialogIcon::Question:
      [alert setIcon:_questionIcon];
      break;
  }

  auto result = [alert runModal];

  if (buttons == DialogButtons::Ok) {
    if (result == NSAlertFirstButtonReturn) return DialogResult::Ok;
    else return DialogResult::Cancel;
  }

  if (buttons == DialogButtons::OkCancel) {
    switch (result) {
      case NSAlertFirstButtonReturn:
        return DialogResult::Ok;
      case NSAlertSecondButtonReturn:
      default:
        return DialogResult::Cancel;
    }
  }

  if (buttons == DialogButtons::YesNo) {
    switch (result) {
      case NSAlertFirstButtonReturn:
        return DialogResult::Yes;
      case NSAlertSecondButtonReturn:
        return DialogResult::No;
      default:
        return DialogResult::Cancel;
    }
  }

  if (buttons == DialogButtons::YesNoCancel) {
    switch (result) {
      case NSAlertFirstButtonReturn:
        return DialogResult::Yes;
      case NSAlertSecondButtonReturn:
        return DialogResult::No;
      case NSAlertThirdButtonReturn:
      default:
        return DialogResult::Cancel;
    }
  }

  if (buttons == DialogButtons::RetryCancel) {
    switch (result) {
      case NSAlertFirstButtonReturn:
        return DialogResult::Retry;
      case NSAlertSecondButtonReturn:
      default:
        return DialogResult::Cancel;
    }
  }

  if (buttons == DialogButtons::AbortRetryIgnore) {
    switch (result) {
      case NSAlertFirstButtonReturn:
        return DialogResult::Abort;
      case NSAlertSecondButtonReturn:
        return DialogResult::Retry;
      case NSAlertThirdButtonReturn:
      default:
        return DialogResult::Ignore;
    }
  }

  return DialogResult::Cancel;
}
#endif