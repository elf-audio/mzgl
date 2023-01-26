

/*

Originally compiled with
g++ main.cpp `pkg-config --libs --cflags gtk+-2.0`


*/
#include "linuxUtil.h"
#include "log.h"

#include <stdio.h>

#include <gtk/gtk.h>

using namespace std;

void linuxSaveFileDialog(string msg, string defaultFileName, function<void(string, bool)> completionCallback) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new(
		msg.c_str(), nullptr, GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, nullptr);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);

	gtk_file_chooser_set_current_name(chooser, defaultFileName.c_str());

	auto res = gtk_dialog_run(GTK_DIALOG(dialog));
	while (g_main_context_iteration(nullptr, false))
		;

	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename = gtk_file_chooser_get_filename(chooser);

		completionCallback(filename, true);
		g_free(filename);
	} else {
		completionCallback("", false);
	}
	gtk_widget_destroy(dialog);
}

void linuxLoadFileDialog(string msg, const vector<string> &allowedExtensions, function<void(string, bool)> completionCallback) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new(
		msg.c_str(), nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, nullptr);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

	if (!allowedExtensions.empty()) {
		auto *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "allowed files");
		for (const auto &ext: allowedExtensions) {
			string glob = "*." + ext;
			gtk_file_filter_add_pattern(filter, glob.c_str());
		}
		gtk_file_chooser_add_filter(chooser, filter);
	}

	auto res = gtk_dialog_run(GTK_DIALOG(dialog));
	while (g_main_context_iteration(nullptr, false))
		;

	char *filename = nullptr;
	bool success = false;

	if (res == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(chooser);
		success = true;
	}

	gtk_widget_destroy(dialog);
	while (g_main_context_iteration(nullptr, false))
		; // to handle outstanding gtk events

	completionCallback(success ? filename : "", success);
	g_free(filename);
}

void linuxTextboxDialog(std::string title, std::string msg, std::string text, function<void(string, bool)> completionCallback) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(title.c_str(), nullptr, (GtkDialogFlags) 0, "OK", 1, "Cancel", 2, nullptr);

	GtkWidget *label = gtk_label_new(msg.c_str());

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);

	GtkWidget *textbox = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(textbox), 0);
	gtk_entry_set_text(GTK_ENTRY(textbox), text.c_str());
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), textbox);

	gtk_widget_show_all(dialog);

	auto result = gtk_dialog_run(GTK_DIALOG(dialog));
	while (g_main_context_iteration(nullptr, false))
		;
	string s = "";
	if (result == 1) {
		s = gtk_entry_get_text(GTK_ENTRY(textbox));
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
	while (g_main_context_iteration(nullptr, false))
		;

	completionCallback(s, result == 1);
}

void linuxAlertDialog(string title, string msg) {
	auto type = GTK_MESSAGE_INFO;
	auto buttons = GTK_BUTTONS_OK;
	GtkWidget *dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, type, buttons, "%s", msg.c_str());
	gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	while (g_main_context_iteration(nullptr, false))
		;
}

void linuxConfirmDialog(std::string title, std::string msg, std::function<void()> okPressed, std::function<void()> cancelPressed) {
	auto type = GTK_MESSAGE_QUESTION;
	auto buttons = GTK_BUTTONS_OK_CANCEL;
	GtkWidget *dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, type, buttons, "%s", msg.c_str());
	gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
	auto result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	while (g_main_context_iteration(nullptr, false))
		;
	if (result == GTK_RESPONSE_OK) {
		okPressed();
	} else {
		cancelPressed();
	}
}

void linuxTwoOptionCancelDialog(std::string title,
								std::string msg,
								std::string buttonOneText,
								std::function<void()> buttonOnePressed,
								std::string buttonTwoText,
								std::function<void()> buttonTwoPressed,
								std::function<void()> cancelPressed) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(title.c_str(),
													nullptr,
													(GtkDialogFlags) 0,
													buttonOneText.c_str(),
													1,
													buttonTwoText.c_str(),
													2,
													"Cancel",
													3,

													nullptr);

	GtkWidget *label = gtk_label_new(msg.c_str());

	//    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
	gtk_widget_show_all(dialog);

	auto result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	while (g_main_context_iteration(nullptr, false))
		;
	if (result == 1) {
		buttonOnePressed();
	} else if (result == 2) {
		buttonTwoPressed();
	} else {
		cancelPressed();
	}
}

void linuxTwoOptionDialog(std::string title,
						  std::string msg,
						  std::string buttonOneText,
						  std::function<void()> buttonOnePressed,
						  std::string buttonTwoText,
						  std::function<void()> buttonTwoPressed) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(title.c_str(),
													nullptr,
													(GtkDialogFlags) 0,
													buttonOneText.c_str(),
													1,
													buttonTwoText.c_str(),
													2,

													nullptr);

	GtkWidget *label = gtk_label_new(msg.c_str());

	//    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
	gtk_widget_show_all(dialog);

	auto result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	while (g_main_context_iteration(nullptr, false))
		;
	if (result == 1) {
		buttonOnePressed();
	} else if (result == 2) {
		buttonTwoPressed();
	}
}

void linuxThreeOptionCancelDialog(std::string title,
								  std::string msg,
								  std::string buttonOneText,
								  std::function<void()> buttonOnePressed,
								  std::string buttonTwoText,
								  std::function<void()> buttonTwoPressed,
								  std::string buttonThreeText,
								  std::function<void()> buttonThreePressed,
								  std::function<void()> cancelPressed) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons(title.c_str(),
													nullptr,
													(GtkDialogFlags) 0,
													buttonOneText.c_str(),
													1,
													buttonTwoText.c_str(),
													2,
													buttonThreeText.c_str(),
													3,
													"Cancel",
													4,

													nullptr);

	GtkWidget *label = gtk_label_new(msg.c_str());
	//   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
	gtk_widget_show_all(dialog);

	auto result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	while (g_main_context_iteration(nullptr, false))
		;
	if (result == 1) {
		buttonOnePressed();
	} else if (result == 2) {
		buttonTwoPressed();
	} else if (result == 3) {
		buttonThreePressed();
	} else {
		cancelPressed();
	}
}
