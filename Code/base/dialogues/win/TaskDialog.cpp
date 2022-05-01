
#ifdef _WIN32

#include <algorithm>
#include <base/dialogues/win/TaskDialog.h>
#include <iterator>
#include <string>

#include <Commctrl.h>
#include <Windows.h>

namespace Base
{
namespace
{
constexpr HRESULT kTimeoutErrorCode = E_ABORT;
} // namespace

TaskDialog::TaskDialog(HMODULE aResourceModule, const wchar_t* apTitle, const wchar_t* apMessage,
                       const wchar_t* apMoreContext, const wchar_t* apOptionalDetails)
    : m_targetModule(aResourceModule), m_pWindowTitle(apTitle), m_pMessage(apMessage), m_pDetails(apOptionalDetails),
      m_pContext(apMoreContext)
{
}

TaskDialog::~TaskDialog() = default;

void TaskDialog::AppendButton(int button_id, const wchar_t* button_text)
{
    dialog_buttons_.emplace_back(button_id, button_text);
}

HRESULT CALLBACK TaskDialog::TaskDialogCallbackProc(HWND aHwnd, UINT notificationCode, WPARAM w_param, LPARAM lParam,
                                                    LONG_PTR ref_data)
{
    switch (notificationCode)
    {
    case TDN_CREATED:
        // After the dialog has been created, but before it is visible, set its
        // z-order so it will be a top-most window and have always on top behavior.
        SetWindowPos(aHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;
    case TDN_HYPERLINK_CLICKED:
        ShellExecuteW(nullptr, L"open", (LPCWSTR)lParam, nullptr, nullptr, SW_NORMAL);
        break;
    case TDN_TIMER: {
        TaskDialog* pDialog = reinterpret_cast<TaskDialog*>(ref_data);
        // Ensure the window is visible before checking if it is in the foreground.
        if (!IsWindowVisible(aHwnd))
        {
            ShowWindow(aHwnd, SW_SHOWNORMAL);
        }

        // Attempt to bring the dialog window to the foreground if needed.  If the
        // window is in the background and cannot be brought forward, this call will
        // flash the placeholder on the taskbar.  Do not call SetForegroundWindow()
        // multiple times as it will cause annoying flashing for the user.
        if (aHwnd == GetForegroundWindow())
        {
            pDialog->m_inFocus = true;
        }
        else if (pDialog->m_inFocus)
        {
            SetForegroundWindow(aHwnd);
            pDialog->m_inFocus = false;
        }

        if (!pDialog->m_inFocus)
        {
            // Ensure the dialog is always at the top of the top-most window stack,
            // even if it doesn't have focus, so the user can always see it.
            BringWindowToTop(aHwnd);
        }
        break;
    }
    default:
        break;
    }

    return S_OK;
}

int TaskDialog::Show(int aIconId)
{
    std::vector<TASKDIALOG_BUTTON> buttons;
    std::transform(dialog_buttons_.begin(), dialog_buttons_.end(), std::back_inserter(buttons),
                   [](const std::pair<int, std::wstring>& button) -> TASKDIALOG_BUTTON {
                       return {button.first, button.second.c_str()};
                   });

    TASKDIALOGCONFIG dialogConfig{};
    dialogConfig.cbSize = sizeof(dialogConfig);
    dialogConfig.hInstance = m_targetModule;
    dialogConfig.pszWindowTitle = m_pWindowTitle;
    dialogConfig.pszMainInstruction = m_pMessage;
    dialogConfig.pszExpandedInformation = m_pDetails;
    dialogConfig.pszContent = m_pContext;
    dialogConfig.pszMainIcon = MAKEINTRESOURCEW(aIconId);
    dialogConfig.dwFlags =
        TDF_ENABLE_HYPERLINKS | TDF_EXPAND_FOOTER_AREA | TDF_USE_COMMAND_LINKS | TDF_EXPANDED_BY_DEFAULT;
    dialogConfig.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    dialogConfig.pfCallback = &TaskDialogCallbackProc;
    dialogConfig.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
    dialogConfig.cButtons = buttons.size();
    dialogConfig.pButtons = buttons.data();
    dialogConfig.nDefaultButton = m_defaultButton;

    int button_result = IDCANCEL;
    HRESULT hr = TaskDialogIndirect(&dialogConfig, &button_result, nullptr, nullptr);
    return button_result;
}
} // namespace Base

#endif
