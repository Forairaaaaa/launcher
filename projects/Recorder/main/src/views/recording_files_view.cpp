#include "views/recording_files_view.hpp"
#include "assets/assets.h"
#include <core/animation/animate_value/animate_value.hpp>
#include <core/animation/generators/generators.hpp>
#include <lvgl/lvgl_cpp/label.hpp>
#include <widget/select_menu/smooth_selector.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace recorder {

namespace {

constexpr int32_t kMenuWidth              = 320;
constexpr int32_t kMenuHeight             = 123;
constexpr int32_t kMenuSelectedX          = 27;
constexpr int32_t kMenuSelectedY          = 9;
constexpr int32_t kMenuItemMinWidth       = 82;
constexpr int32_t kMenuItemMaxWidth       = 262;
constexpr int32_t kMenuItemHeight         = 24;
constexpr int32_t kMenuItemPitch          = 41;
constexpr int32_t kMenuTextPaddingLeft    = 11;
constexpr int32_t kMenuTextPaddingRight   = 11;
constexpr int32_t kMenuSelectorRadius     = 8;
constexpr int32_t kMenuCameraPaddingY     = 9;
constexpr int32_t kDividerX               = 32;
constexpr int32_t kDividerY               = 123;
constexpr int32_t kDividerWidth           = 256;
constexpr int32_t kDividerHeight          = 2;
constexpr uint32_t kDividerColor          = 0x2B2B2B;
constexpr uint32_t kSelectorColor         = 0x333333;
constexpr uint32_t kTextColor             = 0xFFFFFF;
constexpr uint32_t kEmptyTextColor        = 0x666666;
constexpr int32_t kScrollBarX             = 301;
constexpr int32_t kScrollBarY             = 12;
constexpr int32_t kScrollBarWidth         = 3;
constexpr int32_t kScrollBarHeight        = 100;
constexpr int32_t kScrollBarThumbHeight   = 14;
constexpr int32_t kScrollBarRadius        = 1;
constexpr uint32_t kScrollBarColor        = 0x2B2B2B;
constexpr uint32_t kScrollBarThumbColor   = 0x848484;
constexpr int32_t kDeleteDialogWidth      = 258;
constexpr int32_t kDeleteDialogHeight     = 100;
constexpr int32_t kDeleteDialogY          = -21;
constexpr int32_t kDeleteDialogRadius     = 14;
constexpr int32_t kDeleteNameAreaWidth    = 238;
constexpr int32_t kDeleteNameAreaHeight   = 28;
constexpr int32_t kDeleteNameAreaRadius   = 5;
constexpr int32_t kDeleteButtonHeight     = 23;
constexpr int32_t kDeleteButtonRadius     = 5;
constexpr int32_t kDeletePromptCenterX    = 0;
constexpr int32_t kDeletePromptCenterY    = -32;
constexpr int32_t kDeleteNameAreaCenterX  = 0;
constexpr int32_t kDeleteNameAreaCenterY  = -5;
constexpr int32_t kDeleteButtonCenterY    = 28;
constexpr int32_t kDeleteOpenOriginX      = 0;
constexpr int32_t kDeleteOpenOriginY      = -150;
constexpr int32_t kDeleteOpenOriginWidth  = 180;
constexpr int32_t kDeleteOpenOriginHeight = 120;
constexpr int32_t kDeleteCloseTargetX     = 112;
constexpr int32_t kDeleteCloseTargetY     = 128;
constexpr int32_t kDeleteCloseTargetSize  = 18;
constexpr float kSelectorMoveDuration     = 0.32f;
constexpr float kSelectorMoveBounce       = 0.30f;
constexpr float kSelectorShapeDuration    = 0.30f;
constexpr float kSelectorShapeBounce      = 0.18f;
constexpr float kCameraDuration           = 0.44f;
constexpr uint32_t kMenuRenderIntervalMs  = 16;

std::string durationDisplayText(uint32_t durationSec)
{
    const uint32_t hours   = durationSec / 3600;
    const uint32_t minutes = durationSec / 60 % 60;
    const uint32_t seconds = durationSec % 60;

    if (hours > 0) {
        return std::to_string(hours) + "h" + std::to_string(minutes) + "m";
    }
    if (minutes > 0) {
        return std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
    }
    return std::to_string(seconds) + "s";
}

std::string fileDisplayName(const RecordingFile& file)
{
    const size_t slash = file.path.find_last_of("/\\");
    std::string name   = slash == std::string::npos ? file.path : file.path.substr(slash + 1);

    if (name.size() >= 4) {
        std::string ext = name.substr(name.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".wav") {
            name = name.substr(0, name.size() - 4);
        }
    }

    return name + " (" + durationDisplayText(file.durationSec) + ")";
}

int32_t textWidth(const std::string& text)
{
    lv_point_t size{};
    lv_text_get_size(&size, text.c_str(), &font_chivo_medium_14, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

int32_t optionWidth(const std::string& text)
{
    return std::clamp(textWidth(text) + kMenuTextPaddingLeft + kMenuTextPaddingRight, kMenuItemMinWidth,
                      kMenuItemMaxWidth);
}

}  // namespace

class RecordingFilesMenu : public smooth_ui_toolkit::SmoothSelectorMenu {
public:
    explicit RecordingFilesMenu(lv_obj_t* parent)
        : _panel(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent)),
          _selector(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr())),
          _empty_label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_panel->raw_ptr())),
          _scroll_bar(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr())),
          _scroll_thumb(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr()))
    {
        _panel->setSize(kMenuWidth, kMenuHeight);
        _panel->align(LV_ALIGN_TOP_MID, 0, 6);
        _panel->setBgOpa(LV_OPA_TRANSP);
        _panel->setBorderWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        _selector->setBgColor(lv_color_hex(kSelectorColor));
        _selector->setBgOpa(LV_OPA_COVER);
        _selector->setRadius(kMenuSelectorRadius);
        _selector->setBorderWidth(0);
        _selector->setShadowWidth(0);
        _selector->setPaddingAll(0);
        _selector->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _selector->addFlag(LV_OBJ_FLAG_HIDDEN);

        _empty_label->setText("No recordings");
        _empty_label->setTextFont(&font_chivo_medium_14);
        _empty_label->setTextColor(lv_color_hex(kEmptyTextColor));
        _empty_label->setTextAlign(LV_TEXT_ALIGN_CENTER);
        _empty_label->setSize(kMenuWidth, LV_SIZE_CONTENT);
        _empty_label->align(LV_ALIGN_CENTER, 0, -8);
        _empty_label->addFlag(LV_OBJ_FLAG_HIDDEN);

        setupScrollBar();
        setupAnimation();
        setConfig().moveInLoop        = true;
        setConfig().renderInterval    = kMenuRenderIntervalMs;
        setConfig().readInputInterval = 0;
        setCameraSize(kMenuWidth, kMenuHeight);
    }

    void setFiles(const std::vector<RecordingFile>& files, int selectedIndex)
    {
        _rows.clear();
        _data.option_list.clear();
        _data.selected_option_index = 0;

        if (files.empty()) {
            _selector->addFlag(LV_OBJ_FLAG_HIDDEN);
            _empty_label->removeFlag(LV_OBJ_FLAG_HIDDEN);
            _scroll_bar->addFlag(LV_OBJ_FLAG_HIDDEN);
            _scroll_thumb->addFlag(LV_OBJ_FLAG_HIDDEN);
            return;
        }

        _selector->removeFlag(LV_OBJ_FLAG_HIDDEN);
        _empty_label->addFlag(LV_OBJ_FLAG_HIDDEN);

        _rows.reserve(files.size());
        for (size_t i = 0; i < files.size(); ++i) {
            std::string name    = fileDisplayName(files[i]);
            const int32_t width = optionWidth(name);
            addOption({{static_cast<float>(kMenuSelectedX),
                        static_cast<float>(kMenuSelectedY + static_cast<int32_t>(i) * kMenuItemPitch),
                        static_cast<float>(width), static_cast<float>(kMenuItemHeight)},
                       nullptr});
            _rows.push_back(std::make_unique<Row>(_panel->raw_ptr(), std::move(name), width));
        }

        jumpToInstant(clampIndex(selectedIndex));
        render();
    }

    void setSelectedIndex(int index)
    {
        if (_data.option_list.empty()) {
            return;
        }
        moveToWithCamera(clampIndex(index));
    }

    void update(uint32_t nowMs) override
    {
        smooth_ui_toolkit::SmoothSelectorMenu::update(nowMs);
        if (!_data.option_list.empty()) {
            render();
        }
    }

private:
    struct Row {
        Row(lv_obj_t* parent, std::string name, int32_t width)
            : container(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent)),
              label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(container->raw_ptr())),
              itemWidth(width),
              text(std::move(name))
        {
            container->setSize(itemWidth, kMenuItemHeight);
            container->setBgOpa(LV_OPA_TRANSP);
            container->setBorderWidth(0);
            container->setShadowWidth(0);
            container->setPaddingAll(0);
            container->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
            container->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

            label->setText(text);
            label->setTextFont(&font_chivo_medium_14);
            label->setTextColor(lv_color_hex(kTextColor));
            label->setLongMode(LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
            label->setSize(itemWidth - kMenuTextPaddingLeft - kMenuTextPaddingRight, LV_SIZE_CONTENT);
            label->align(LV_ALIGN_LEFT_MID, kMenuTextPaddingLeft, 0);
        }

        std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> container;
        std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> label;
        int32_t itemWidth = 0;
        std::string text;
    };

    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _panel;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _selector;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _empty_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _scroll_bar;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _scroll_thumb;
    std::vector<std::unique_ptr<Row>> _rows;

    int clampIndex(int index) const
    {
        if (_data.option_list.empty()) {
            return 0;
        }
        return std::clamp(index, 0, static_cast<int>(_data.option_list.size()) - 1);
    }

    void jumpToInstant(int index)
    {
        if (_data.option_list.empty()) {
            return;
        }

        _data.selected_option_index = clampIndex(index);
        const auto& keyframe        = _data.option_list[_data.selected_option_index].keyframe;
        getSelectorPostion().teleport(keyframe.x, keyframe.y);
        getSelectorShape().teleport(keyframe.width, keyframe.height);
        getCamera().teleport(0, cameraYFor(keyframe));
    }

    void moveToWithCamera(int index)
    {
        _data.selected_option_index = clampIndex(index);
        _update_selector_keyframe();
        _update_camera_keyframe();
    }

    void _update_camera_keyframe() override
    {
        if (_data.option_list.empty()) {
            return;
        }

        const auto& keyframe = getSelectedKeyframe();
        getCamera().move(0, cameraYFor(keyframe));
    }

    int32_t cameraYFor(const smooth_ui_toolkit::Vector4& keyframe)
    {
        int32_t offset           = static_cast<int32_t>(std::round(getCameraOffset().y));
        const int32_t top        = static_cast<int32_t>(std::round(keyframe.y));
        const int32_t bottom     = top + static_cast<int32_t>(std::round(keyframe.height));
        const int32_t max_offset = maxCameraY();

        if (top - offset < kMenuCameraPaddingY) {
            offset = top - kMenuCameraPaddingY;
        } else if (bottom - offset > kMenuHeight - kMenuCameraPaddingY) {
            offset = bottom - kMenuHeight + kMenuCameraPaddingY;
        }

        return std::clamp(offset, 0, max_offset);
    }

    int32_t maxCameraY() const
    {
        if (_data.option_list.empty()) {
            return 0;
        }

        const auto& keyframe         = _data.option_list.back().keyframe;
        const int32_t content_bottom = static_cast<int32_t>(std::round(keyframe.y + keyframe.height));
        return std::max(0, content_bottom + kMenuCameraPaddingY - kMenuHeight);
    }

    void setupScrollBar()
    {
        _scroll_bar->setSize(kScrollBarWidth, kScrollBarHeight);
        _scroll_bar->setPos(kScrollBarX, kScrollBarY);
        _scroll_bar->setBgColor(lv_color_hex(kScrollBarColor));
        _scroll_bar->setBgOpa(LV_OPA_COVER);
        _scroll_bar->setRadius(kScrollBarRadius);
        _scroll_bar->setBorderWidth(0);
        _scroll_bar->setShadowWidth(0);
        _scroll_bar->setPaddingAll(0);
        _scroll_bar->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _scroll_bar->addFlag(LV_OBJ_FLAG_HIDDEN);

        _scroll_thumb->setSize(kScrollBarWidth, kScrollBarThumbHeight);
        _scroll_thumb->setPos(kScrollBarX, kScrollBarY);
        _scroll_thumb->setBgColor(lv_color_hex(kScrollBarThumbColor));
        _scroll_thumb->setBgOpa(LV_OPA_COVER);
        _scroll_thumb->setRadius(kScrollBarRadius);
        _scroll_thumb->setBorderWidth(0);
        _scroll_thumb->setShadowWidth(0);
        _scroll_thumb->setPaddingAll(0);
        _scroll_thumb->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _scroll_thumb->addFlag(LV_OBJ_FLAG_HIDDEN);
    }

    void setupAnimation()
    {
        auto& selector_position_options          = getSelectorPostion().x.springOptions();
        selector_position_options.visualDuration = kSelectorMoveDuration;
        selector_position_options.bounce         = kSelectorMoveBounce;
        getSelectorPostion().y.springOptions()   = selector_position_options;

        auto& selector_shape_options          = getSelectorShape().x.springOptions();
        selector_shape_options.visualDuration = kSelectorShapeDuration;
        selector_shape_options.bounce         = kSelectorShapeBounce;
        getSelectorShape().y.springOptions()  = selector_shape_options;

        auto& camera_options          = getCamera().y.springOptions();
        camera_options.visualDuration = kCameraDuration;
        camera_options.bounce         = 0.0f;
        getCamera().x.springOptions() = camera_options;
    }

    void render()
    {
        const auto selector = getSelectorCurrentFrame();
        _selector->setSize(static_cast<int32_t>(std::round(selector.width)),
                           static_cast<int32_t>(std::round(selector.height)));
        const int32_t camera_x_offset = -static_cast<int32_t>(std::round(getCameraOffset().x));
        const int32_t camera_y_offset = -static_cast<int32_t>(std::round(getCameraOffset().y));
        _selector->setPos(static_cast<int32_t>(std::round(selector.x)) + camera_x_offset,
                          static_cast<int32_t>(std::round(selector.y)) + camera_y_offset);

        for (size_t i = 0; i < _rows.size() && i < _data.option_list.size(); ++i) {
            const auto& keyframe = _data.option_list[i].keyframe;
            auto& row            = *_rows[i];
            row.container->setSize(row.itemWidth, kMenuItemHeight);
            row.container->setPos(static_cast<int32_t>(std::round(keyframe.x)) + camera_x_offset,
                                  static_cast<int32_t>(std::round(keyframe.y)) + camera_y_offset);
        }

        renderScrollBar();
    }

    void renderScrollBar()
    {
        const int32_t max_offset = maxCameraY();
        if (max_offset <= 0) {
            _scroll_bar->addFlag(LV_OBJ_FLAG_HIDDEN);
            _scroll_thumb->addFlag(LV_OBJ_FLAG_HIDDEN);
            return;
        }

        const float offset         = std::clamp(getCameraOffset().y, 0.0f, static_cast<float>(max_offset));
        const float progress       = offset / static_cast<float>(max_offset);
        const int32_t thumb_travel = kScrollBarHeight - kScrollBarThumbHeight;
        const int32_t thumb_y      = kScrollBarY + static_cast<int32_t>(std::round(progress * thumb_travel));

        _scroll_bar->removeFlag(LV_OBJ_FLAG_HIDDEN);
        _scroll_thumb->removeFlag(LV_OBJ_FLAG_HIDDEN);
        _scroll_thumb->setPos(kScrollBarX, thumb_y);
    }
};

class RecordingFilesView::DeleteConfirmDialog {
public:
    DeleteConfirmDialog(lv_obj_t* parent, FilesViewModel& view_model)
        : _view_model(view_model),
          _panel(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent)),
          _prompt_label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_panel->raw_ptr())),
          _name_label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_panel->raw_ptr())),
          _cancel_button(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr())),
          _confirm_button(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(_panel->raw_ptr())),
          _cancel_label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_cancel_button->raw_ptr())),
          _confirm_label(std::make_unique<smooth_ui_toolkit::lvgl_cpp::Label>(_confirm_button->raw_ptr())),
          _x(kDeleteOpenOriginX),
          _y(kDeleteOpenOriginY),
          _width(kDeleteOpenOriginWidth),
          _height(kDeleteOpenOriginHeight)
    {
        configureOpenAnimation();

        applyAnimatedValue();
        _panel->setBgColor(lv_color_hex(0x474747));
        _panel->setBgOpa(LV_OPA_COVER);
        _panel->setRadius(kDeleteDialogRadius);
        _panel->setBorderWidth(0);
        _panel->setShadowWidth(0);
        _panel->setPaddingAll(0);
        _panel->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _panel->addFlag(LV_OBJ_FLAG_HIDDEN);

        setupPrompt();
        setupNameArea();
        setupButton(*_cancel_button, *_cancel_label, -46, 102, "ESC: Cancel", lv_color_hex(0x6D6D6D),
                    lv_color_hex(0xF3F3F3), onCancelClicked);
        setupButton(*_confirm_button, *_confirm_label, 67, 106, "Enter: Delete", lv_color_hex(0xC33630),
                    lv_color_hex(0xFFECEC), onConfirmClicked);
    }

    void setPending(const PendingDeleteRecordingFile& pending)
    {
        if (pending.active) {
            setName(fileDisplayName(pending.file));
            showControls();
            _panel->removeFlag(LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(_panel->raw_ptr());
            _visible = true;
            configureOpenAnimation();
            _x.teleport(kDeleteOpenOriginX);
            _y.teleport(kDeleteOpenOriginY);
            _width.teleport(kDeleteOpenOriginWidth);
            _height.teleport(kDeleteOpenOriginHeight);
            applyAnimatedValue();
            _x.move(0);
            _y.move(kDeleteDialogY);
            _width.move(kDeleteDialogWidth);
            _height.move(kDeleteDialogHeight);
        } else {
            _visible = false;
            close(_view_model.deleteRecordingCloseAction());
        }
    }

    void tick()
    {
        _x.update();
        _y.update();
        _width.update();
        _height.update();
        applyAnimatedValue();

        if (!_visible && _x.done() && _y.done() && _width.done() && _height.done()) {
            _panel->addFlag(LV_OBJ_FLAG_HIDDEN);
        }
    }

private:
    FilesViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _panel;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _prompt_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _name_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _cancel_button;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _confirm_button;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _cancel_label;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Label> _confirm_label;
    smooth_ui_toolkit::AnimateValue _x;
    smooth_ui_toolkit::AnimateValue _y;
    smooth_ui_toolkit::AnimateValue _width;
    smooth_ui_toolkit::AnimateValue _height;
    bool _visible = false;

    static void setupAnimation(smooth_ui_toolkit::AnimateValue& value, float duration, float bounce)
    {
        value.springOptions().visualDuration = duration;
        value.springOptions().bounce         = bounce;
    }

    void configureOpenAnimation()
    {
        setupAnimation(_x, 0.35, 0.4f);
        setupAnimation(_y, 0.35, 0.3f);
        setupAnimation(_width, 0.35, 0.2f);
        setupAnimation(_height, 0.35, 0.2f);
        _y.delay = 0.0;
    }

    void configureCloseAnimation()
    {
        setupAnimation(_x, 0.4, 0.18f);
        setupAnimation(_y, 0.5, 0.18f);
        setupAnimation(_width, 0.4, 0.12f);
        setupAnimation(_height, 0.4, 0.12f);
        _y.delay = 0.2;
    }

    void close(DeleteRecordingCloseAction action)
    {
        if (action == DeleteRecordingCloseAction::Cancel) {
            configureOpenAnimation();
            _x.move(kDeleteOpenOriginX);
            _y.move(kDeleteOpenOriginY);
            _width.move(kDeleteOpenOriginWidth);
            _height.move(kDeleteOpenOriginHeight);
            return;
        }

        hideControls();
        configureCloseAnimation();
        _x.move(kDeleteCloseTargetX);
        _y.move(kDeleteCloseTargetY);
        _width.move(kDeleteCloseTargetSize);
        _height.move(kDeleteCloseTargetSize);
    }

    void applyAnimatedValue()
    {
        _panel->setSize(static_cast<int32_t>(std::round(_width.directValue())),
                        static_cast<int32_t>(std::round(_height.directValue())));
        _panel->align(LV_ALIGN_CENTER, static_cast<int32_t>(std::round(_x.directValue())),
                      static_cast<int32_t>(std::round(_y.directValue())));
    }

    void setupPrompt()
    {
        _prompt_label->setText("Delete recording?");
        _prompt_label->setTextFont(&font_chivo_medium_14);
        _prompt_label->setTextColor(lv_color_hex(0xBCBCBC));
        _prompt_label->setTextAlign(LV_TEXT_ALIGN_LEFT);
        _prompt_label->setSize(220, LV_SIZE_CONTENT);
        _prompt_label->align(LV_ALIGN_CENTER, kDeletePromptCenterX, kDeletePromptCenterY);
    }

    void setupNameArea()
    {
        _name_label->setText("");
        _name_label->setTextFont(&font_chivo_medium_14);
        _name_label->setTextColor(lv_color_hex(0xFFFFFF));
        _name_label->setLongMode(LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        _name_label->setSize(kDeleteNameAreaWidth - 18, LV_SIZE_CONTENT);
        _name_label->align(LV_ALIGN_CENTER, kDeleteNameAreaCenterX, kDeleteNameAreaCenterY);
    }

    void setupButton(smooth_ui_toolkit::lvgl_cpp::Container& button, smooth_ui_toolkit::lvgl_cpp::Label& label,
                     int32_t x, int32_t width, const char* text, lv_color_t bg_color, lv_color_t label_color,
                     lv_event_cb_t callback)
    {
        button.setSize(width, kDeleteButtonHeight);
        button.align(LV_ALIGN_CENTER, x, kDeleteButtonCenterY);
        button.setBgColor(bg_color);
        button.setBgOpa(LV_OPA_COVER);
        button.setRadius(kDeleteButtonRadius);
        button.setBorderWidth(0);
        button.setShadowWidth(0);
        button.setPaddingAll(0);
        button.removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        button.addFlag(LV_OBJ_FLAG_CLICKABLE);
        button.addEventCb(callback, LV_EVENT_CLICKED, this);

        label.setText(text);
        label.setTextFont(&font_chivo_medium_14);
        label.setTextColor(label_color);
        label.setTextAlign(LV_TEXT_ALIGN_CENTER);
        label.center();
    }

    void setName(const std::string& name)
    {
        _name_label->setText(name);
    }

    void showControls()
    {
        _prompt_label->removeFlag(LV_OBJ_FLAG_HIDDEN);
        _name_label->removeFlag(LV_OBJ_FLAG_HIDDEN);
        _cancel_button->removeFlag(LV_OBJ_FLAG_HIDDEN);
        _confirm_button->removeFlag(LV_OBJ_FLAG_HIDDEN);
    }

    void hideControls()
    {
        _prompt_label->addFlag(LV_OBJ_FLAG_HIDDEN);
        _name_label->addFlag(LV_OBJ_FLAG_HIDDEN);
        _cancel_button->addFlag(LV_OBJ_FLAG_HIDDEN);
        _confirm_button->addFlag(LV_OBJ_FLAG_HIDDEN);
    }

    static void onCancelClicked(lv_event_t* event)
    {
        auto* self = static_cast<DeleteConfirmDialog*>(lv_event_get_user_data(event));
        if (self) {
            self->_view_model.cancelDeleteRecording();
        }
    }

    static void onConfirmClicked(lv_event_t* event)
    {
        auto* self = static_cast<DeleteConfirmDialog*>(lv_event_get_user_data(event));
        if (self) {
            self->_view_model.confirmDeleteRecording();
        }
    }
};

RecordingFilesView::RecordingFilesView(FilesViewModel& view_model) : _view_model(view_model)
{
}

RecordingFilesView::~RecordingFilesView()
{
    onExit();
}

void RecordingFilesView::onEnter(lv_obj_t* parent)
{
    onExit();

    _root = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent);
    _root->setSize(lv_pct(100), lv_pct(100));
    _root->setBgColor(lv_color_hex(0x000000));
    _root->setBgOpa(LV_OPA_COVER);
    _root->setBorderWidth(0);
    _root->setPaddingAll(0);
    _root->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
    _root->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _menu                  = std::make_unique<RecordingFilesMenu>(_root->raw_ptr());
    _delete_confirm_dialog = std::make_unique<DeleteConfirmDialog>(_root->raw_ptr(), _view_model);

    _key_bar = std::make_unique<BottomKeyBar>(_root->raw_ptr());
    _key_bar->setItems({
        {'4', &image_icon_nav_back},
        {'5', &image_icon_nav_up},
        {'6', &image_icon_nav_down},
        {'7', &image_icon_play},
        {'8', &image_icon_delete},
    });

    _view_model.files().observe(this, onFilesChanged);
    _view_model.selectedIndex().observe(this, onSelectedIndexChanged);
    _view_model.pendingDeleteRecording().observe(this, onPendingDeleteRecordingChanged);
}

void RecordingFilesView::onExit()
{
    _view_model.pendingDeleteRecording().removeObserver();
    _view_model.selectedIndex().removeObserver();
    _view_model.files().removeObserver();

    _key_bar.reset();
    _delete_confirm_dialog.reset();
    _menu.reset();
    _root.reset();
}

void RecordingFilesView::tick(uint32_t nowMs)
{
    if (_menu) {
        _menu->update(nowMs);
    }
    if (_key_bar) {
        _key_bar->tick();
    }
    if (_delete_confirm_dialog) {
        _delete_confirm_dialog->tick();
    }
}

void RecordingFilesView::renderFiles(const std::vector<RecordingFile>& files)
{
    if (_menu) {
        _menu->setFiles(files, _view_model.selectedIndex().get());
    }
}

void RecordingFilesView::renderSelectedIndex(int index)
{
    if (_menu) {
        _menu->setSelectedIndex(index);
    }
}

void RecordingFilesView::renderPendingDeleteRecording(const PendingDeleteRecordingFile& pending)
{
    if (_delete_confirm_dialog) {
        _delete_confirm_dialog->setPending(pending);
    }
}

void RecordingFilesView::onFilesChanged(void* context, const std::vector<RecordingFile>& files)
{
    auto* self = static_cast<RecordingFilesView*>(context);
    if (self) {
        self->renderFiles(files);
    }
}

void RecordingFilesView::onSelectedIndexChanged(void* context, const int& index)
{
    auto* self = static_cast<RecordingFilesView*>(context);
    if (self) {
        self->renderSelectedIndex(index);
    }
}

void RecordingFilesView::onPendingDeleteRecordingChanged(void* context, const PendingDeleteRecordingFile& pending)
{
    auto* self = static_cast<RecordingFilesView*>(context);
    if (self) {
        self->renderPendingDeleteRecording(pending);
    }
}

}  // namespace recorder
