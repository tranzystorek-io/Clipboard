/*  Clipboard - Cut, copy, and paste anything, anywhere, all from the terminal.
    Copyright (C) 2023 Jackson Huff and other contributors on GitHub.com
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.*/
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#include <clipboard/fork.hpp>
#include <clipboard/gui.hpp>

namespace fs = std::filesystem;

#if !defined(GIT_COMMIT_HASH)
#define GIT_COMMIT_HASH "not available"
#endif

#if !defined(CLIPBOARD_VERSION)
#define CLIPBOARD_VERSION "(version not available)"
#endif

extern Forker forker;

struct Filepath {
    class DataDirectory {
        fs::path self;

    public:
        fs::path raw;
        operator fs::path() { return self; }
        operator fs::path() const { return self; }
        auto operator=(const auto& other) { return self = other; }
        auto operator/(const auto& other) { return self / other; }
    } data;

    class MetadataDirectory {
        fs::path self;

    public:
        fs::path notes;
        fs::path originals;
        fs::path lock;
        operator fs::path() { return self; }
        operator fs::path() const { return self; }
        auto operator=(const auto& other) { return self = other; }
        auto operator/(const auto& other) { return self / other; }
    } metadata;
    fs::path temporary;
    fs::path persistent;
    fs::path root;
    fs::path home;

    void create() {
        [](const auto&... path) {
            (fs::create_directories(path), ...);
        }(data, metadata, temporary, persistent);
    }
};
extern Filepath path;

enum class CopyPolicy { ReplaceAll, ReplaceOnce, SkipOnce, SkipAll, Unknown };

struct Copying {
    bool is_persistent = false;
    bool use_safe_copy = true;
    CopyPolicy policy = CopyPolicy::Unknown;
    fs::copy_options opts = fs::copy_options::overwrite_existing | fs::copy_options::recursive | fs::copy_options::copy_symlinks;
    std::vector<fs::path> items;
    std::vector<std::pair<std::string, std::error_code>> failedItems;
    std::string buffer;
    std::string mime;
};
extern Copying copying;

extern std::vector<std::string> arguments;

extern std::string clipboard_name;

extern bool output_silent;
extern bool no_color;

enum class ProgressState : int { Done, Active, Cancel };

extern std::condition_variable cv;
extern std::mutex m;
extern std::atomic<ProgressState> progress_state;
static std::thread indicator;

struct Successes {
    std::atomic<unsigned long> files;
    std::atomic<unsigned long> directories;
    std::atomic<unsigned long long> bytes;
};
extern Successes successes;

struct IsTTY {
    bool in = true;
    bool out = true;
    bool err = true;
};
extern IsTTY is_tty;

struct Constants {
    std::string_view clipboard_version = CLIPBOARD_VERSION;
    std::string_view clipboard_commit = GIT_COMMIT_HASH;
    std::string_view data_file_name = "rawdata.clipboard";
    std::string_view default_clipboard_name = "0";
    std::string_view temporary_directory_name = "Clipboard";
    std::string_view persistent_directory_name = ".clipboard";
    std::string_view original_files_name = "originals";
    std::string_view notes_name = "notes";
    std::string_view mime_name = "mime";
    std::string_view lock_name = "lock";
    std::string_view data_directory = "data";
    std::string_view metadata_directory = "metadata";
};
constexpr Constants constants;

enum class Action : unsigned int { Cut, Copy, Paste, Clear, Show, Edit, Add, Remove, Note, Swap };

extern Action action;

enum class IOType : unsigned int { File, Pipe, Text };

extern IOType io_type;

template <typename T, size_t N>
class EnumArray : public std::array<T, N> {
public:
    T& operator[](Action index) { return std::array<T, N>::operator[](static_cast<unsigned int>(index)); } // switch to std::to_underlying when available
};

extern EnumArray<std::string_view, 10> actions;
extern EnumArray<std::string_view, 10> action_shortcuts;
extern EnumArray<std::string_view, 10> doing_action;
extern EnumArray<std::string_view, 10> did_action;

extern std::array<std::pair<std::string_view, std::string_view>, 7> colors;

class TerminalSize {
public:
    size_t rows;
    size_t columns;
    TerminalSize(const unsigned int& rows, const unsigned int& columns) : rows {std::max(1u, rows)}, columns {std::max(1u, columns)} {}
    unsigned int accountRowsFor(const auto&... args) {
        ((rows -= (static_cast<unsigned int>(args) / columns) + 1), ...);
        return columns;
    }
};

static std::string replaceColors(const std::string_view& str, bool colorful = !no_color) {
    std::string temp(str);           // a string to do scratch work on
    for (const auto& key : colors) { // iterate over all the possible colors to replace
        for (size_t i = 0; (i = temp.find(key.first, i)) != std::string::npos; i += key.second.length()) {
            temp.replace(i, key.first.length(), colorful ? key.second : "");
        }
    }
    return temp;
}

void incrementSuccessesForItem(const auto& item) {
    if (fs::is_directory(item))
        successes.directories++;
    else
        successes.files++;
}

class Message {
private:
    std::string_view internal_message;

public:
    Message(const auto& message) : internal_message(std::move(message)) {}
    std::string operator()() const { return std::move(replaceColors(internal_message)); }
};

extern Message help_message;
extern Message check_clipboard_status_message;
extern Message clipboard_item_one_contents_message;
extern Message clipboard_item_many_contents_message;
extern Message clipboard_item_too_many_contents_message;
extern Message clipboard_text_contents_message;
extern Message no_clipboard_contents_message;
extern Message clipboard_action_prompt;
extern Message no_valid_action_message;
extern Message choose_action_items_message;
extern Message fix_redirection_action_message;
extern Message redirection_no_items_message;
extern Message paste_success_message;
extern Message clear_success_message;
extern Message clear_fail_message;
extern Message clipboard_failed_one_message;
extern Message clipboard_failed_many_message;
extern Message and_more_fails_message;
extern Message and_more_items_message;
extern Message fix_problem_message;
extern Message not_enough_storage_message;
extern Message item_already_exists_message;
extern Message bad_response_message;
extern Message working_message;
extern Message cancelled_message;
extern Message byte_success_message;
extern Message one_item_success_message;
extern Message many_files_success_message;
extern Message many_directories_success_message;
extern Message one_file_one_directory_success_message;
extern Message one_file_many_directories_success_message;
extern Message many_files_one_directory_success_message;
extern Message many_files_many_directories_success_message;
extern Message internal_error_message;

void releaseLock();
void setLanguagePT();
void setLanguageTR();
void setLanguageES();
void setupHandlers();
void setLocale();
void showHelpMessage(int& argc, char* argv[]);
void setupItems(int& argc, char* argv[]);
void writeToFile(const fs::path& path, const std::string& content, bool append = false);
void setClipboardName(int& argc, char* argv[]);
void setupVariables(int& argc, char* argv[]);
void createTempDirectory();
void syncWithGUIClipboard(const std::string& text);
void syncWithGUIClipboard(const ClipboardPaths& clipboard);
void showClipboardContents();
void setupAction(int& argc, char* argv[]);
void checkForNoItems();
bool stopIndicator(bool change_condition_variable);
void startIndicator();
void setupIndicator();
void deduplicateItems();
unsigned long long calculateTotalItemSize();
bool stopIndicator(bool change_condition_variable = true);
void checkItemSize();
TerminalSize getTerminalSize();
void clearTempDirectory(bool force_clear);
void copyFiles();
void removeOldFiles();
std::string fileContents(const fs::path& path);
bool userIsARobot();
void pasteFiles();
void clearClipboard();
void performAction();
void updateGUIClipboard();
std::string pipedInContent();
void showFailures();
void showSuccesses();
[[nodiscard]] CopyPolicy userDecision(const std::string& item);
void setTheme(const std::string& theme);

extern ClipboardContent getGUIClipboard();
extern void writeToGUIClipboard(const ClipboardContent& clipboard);
extern const bool GUIClipboardSupportsCut;

namespace PerformAction {
void copyItem(const fs::path& f);
void copy();
void copyText();
void paste();
void pipeIn();
void pipeOut();
void clear();
void show();
void edit();
void addData();
void addText();
void removeFiles();
void removeRegex();
void noteText();
void notePipe();
void swap();
void addFiles();
} // namespace PerformAction