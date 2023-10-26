#ifdef __linux__
#include "Photino.Dialog.h"

enum DialogType {
    OpenFile,
    OpenFolder,
    SaveFile
};

void AddFilters(GtkWidget* dialog, AutoString* filters, int filterCount)
{
    for (int i = 0; i < filterCount; i++) {
        GtkFileFilter *filter = gtk_file_filter_new();
        char *name = strtok(filters[i], "|");
        gtk_file_filter_set_name(filter, name);
        char *patterns = strtok(NULL, "|");
        while (patterns != NULL) {
            gtk_file_filter_add_pattern(filter, patterns);
            patterns = strtok(NULL, ";");
        }
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }
}

AutoString* ShowDialog(DialogType type, AutoString title, AutoString defaultPath, bool multiSelect, AutoString* filters, int filterCount, int* resultCount) {
    GtkFileChooserAction action;
    const char* buttonText;
    switch (type) {
        case OpenFile:
            action = GTK_FILE_CHOOSER_ACTION_OPEN;
            buttonText = "_Open";
            break;
        case OpenFolder:
            action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
            buttonText = "_Select";
            break;
        case SaveFile:
            action = GTK_FILE_CHOOSER_ACTION_SAVE;
            buttonText = "_Save";
            break;
    }

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        title, NULL, action,
        "_Cancel", GTK_RESPONSE_CANCEL, 
        buttonText, GTK_RESPONSE_ACCEPT,
        NULL);

    if (defaultPath != NULL) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), defaultPath);
    }
    if (type == OpenFile || type == OpenFolder) {
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiSelect);
    }
    if (type == SaveFile) {
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    }
    if (type == OpenFile || type == SaveFile) {
        AddFilters(dialog, filters, filterCount);
    }

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res != GTK_RESPONSE_ACCEPT) {
        if (type == OpenFile || type == OpenFolder)
            *resultCount = 0;

        gtk_widget_destroy(dialog);
        return NULL;
    }

    if (type == OpenFile || type == OpenFolder) {
        GSList* pathList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        int count = g_slist_length(pathList);
        char** results = new char*[count];
        for (int i = 0; i < count; i++) {
            results[i] = g_strdup((char*)g_slist_nth_data(pathList, i));
        }
        g_slist_free(pathList);
        *resultCount = count;
        gtk_widget_destroy(dialog);
        return results;
    }
    else {
        char* result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_widget_destroy(dialog);
        return new char*[1] { result };
    }
}

PhotinoDialog::PhotinoDialog() {}

PhotinoDialog::~PhotinoDialog() {}

AutoString* PhotinoDialog::ShowOpenFile(AutoString title, AutoString defaultPath, bool multiSelect, AutoString* filters, int filterCount, int* resultCount)
{
    return ShowDialog(OpenFile, title, defaultPath, multiSelect, filters, filterCount, resultCount);
}

AutoString* PhotinoDialog::ShowOpenFolder(AutoString title, AutoString defaultPath, bool multiSelect, int* resultCount)
{
    return ShowDialog(OpenFolder, title, defaultPath, multiSelect, NULL, 0, resultCount);
}

AutoString PhotinoDialog::ShowSaveFile(AutoString title, AutoString defaultPath, AutoString* filters, int filterCount)
{
    char** result = ShowDialog(SaveFile, title, defaultPath, false, filters, filterCount, NULL);
    if (result != NULL) return result[0];
    return NULL;
}

DialogResult PhotinoDialog::ShowMessage(AutoString title, AutoString text, DialogButtons buttons, DialogIcon icon)
{
    GtkWidget* dialog;
    GtkMessageType type;

    switch (icon) {
        case DialogIcon::Info:
            type = GTK_MESSAGE_INFO;
            break;
        case DialogIcon::Warning:
            type = GTK_MESSAGE_WARNING;
            break;
        case DialogIcon::Error:
            type = GTK_MESSAGE_ERROR;
            break;
        case DialogIcon::Question:
            type = GTK_MESSAGE_QUESTION;
            break;
        default:
            type = GTK_MESSAGE_OTHER;
            break;
    }

    dialog = gtk_message_dialog_new(nullptr,
                                    GTK_DIALOG_MODAL,
                                    type,
                                    GTK_BUTTONS_NONE,
                                    "%s",
                                    title);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), text);

    switch (buttons) {
        case DialogButtons::Ok:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ok", (gint)DialogResult::Ok);
            break;
        case DialogButtons::OkCancel:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ok", (gint)DialogResult::Ok);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", (gint)DialogResult::Cancel);
            break;
        case DialogButtons::YesNo:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Yes", (gint)DialogResult::Yes);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_No", (gint)DialogResult::No);
            break;
        case DialogButtons::YesNoCancel:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Yes", (gint)DialogResult::Yes);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_No", (gint)DialogResult::No);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", (gint)DialogResult::Cancel);
            break;
        case DialogButtons::RetryCancel:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Retry", (gint)DialogResult::Retry);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", (gint)DialogResult::Cancel);
            break;
        case DialogButtons::AbortRetryIgnore:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Abort", (gint)DialogResult::Abort);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Retry", (gint)DialogResult::Retry);
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ignore", (gint)DialogResult::Ignore);
            break;
        default:
            gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ok", (gint)DialogResult::Ok);
            break;
    }

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch (result) {
        case GTK_RESPONSE_CLOSE:
            return DialogResult::Cancel;
        case (gint)DialogResult::Ok:
            return DialogResult::Ok;
        case (gint)DialogResult::Yes:
            return DialogResult::Yes;
        case (gint)DialogResult::No:
            return DialogResult::No;
        case (gint)DialogResult::Cancel:
            return DialogResult::Cancel;
        case (gint)DialogResult::Abort:
            return DialogResult::Abort;
        case (gint)DialogResult::Retry:
            return DialogResult::Retry;
        case (gint)DialogResult::Ignore:
            return DialogResult::Ignore;
        default:
            return DialogResult::Cancel;
    }
}
#endif