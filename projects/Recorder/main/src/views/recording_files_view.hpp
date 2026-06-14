#pragma once

#include "view_models/files_view_model.hpp"
#include "views/bottom_key_bar.hpp"
#include "views/view.hpp"
#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>
#include <vector>

namespace recorder {

class RecordingFilesMenu;

class RecordingFilesView : public View {
public:
    explicit RecordingFilesView(FilesViewModel& view_model);
    ~RecordingFilesView() override;

    RecordingFilesView(const RecordingFilesView&)            = delete;
    RecordingFilesView& operator=(const RecordingFilesView&) = delete;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;
    void tick(uint32_t nowMs) override;

private:
    class DeleteConfirmDialog;

    FilesViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<RecordingFilesMenu> _menu;
    std::unique_ptr<DeleteConfirmDialog> _delete_confirm_dialog;
    std::unique_ptr<BottomKeyBar> _key_bar;

    void renderFiles(const std::vector<RecordingFile>& files);
    void renderSelectedIndex(int index);
    void renderPendingDeleteRecording(const PendingDeleteRecordingFile& pending);
    static void onFilesChanged(void* context, const std::vector<RecordingFile>& files);
    static void onSelectedIndexChanged(void* context, const int& index);
    static void onPendingDeleteRecordingChanged(void* context, const PendingDeleteRecordingFile& pending);
};

}  // namespace recorder
