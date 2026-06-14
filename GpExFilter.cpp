// GpExFilter.cpp
// External filter feature (Ctrl+G): the command-input dialog and the pipe
// plumbing that runs the selected text (or whole file) through an external
// command via "cmd.exe /c <command>" and pastes stdout back into the editor.
//
// Split out of GpMain.cpp to keep the main window translation unit smaller.
// See docs/external-filter-spec.md for the detailed specification.

#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "GpMain.h"
#include "LangManager.h"
#include "GpProc.h"
using namespace ki;
using namespace editwing;

//-------------------------------------------------------------------------
// External filter helper types (file-scope, used only by on_exfilter)
//-------------------------------------------------------------------------

namespace {

struct ExfStdinArgs {
	HANDLE hSrc; // temp file to read
	HANDLE hDst; // stdin pipe write-end
};

static DWORD WINAPI ExfStdinThread(void* arg)
{
	ExfStdinArgs* a = static_cast<ExfStdinArgs*>(arg);
	BYTE buf[8192];
	DWORD n;
	while (ReadFile(a->hSrc, buf, sizeof(buf), &n, NULL) && n > 0)
		WriteFile(a->hDst, buf, n, &n, NULL);
	CloseHandle(a->hSrc);
	CloseHandle(a->hDst); // signals EOF to child
	return 0;
}

struct ExfStderrArgs {
	HANDLE hSrc; // stderr pipe read-end
	BYTE*  buf;
	DWORD  size;
	DWORD  cap;
};

static DWORD WINAPI ExfStderrThread(void* arg)
{
	ExfStderrArgs* a = static_cast<ExfStderrArgs*>(arg);
	BYTE tmp[4096];
	DWORD n;
	while (ReadFile(a->hSrc, tmp, sizeof(tmp), &n, NULL) && n > 0) {
		if (a->size + n > a->cap) {
			DWORD newCap = (a->cap ? a->cap * 2 : 4096) + n;
			BYTE* nb = a->buf
				? (BYTE*)HeapReAlloc(GetProcessHeap(), 0, a->buf, newCap)
				: (BYTE*)HeapAlloc(GetProcessHeap(), 0, newCap);
			if (!nb) break;
			a->buf = nb; a->cap = newCap;
		}
		if (!a->buf) break;
		memcpy(a->buf + a->size, tmp, n);
		a->size += n;
	}
	CloseHandle(a->hSrc);
	return 0;
}

} // namespace

//-------------------------------------------------------------------------

void GreenPadWnd::on_exfilter()
{
	// --- Show command-input dialog ---
	struct ExFilterDlg A_FINAL : public DlgImpl {
		ExFilterDlg(HWND parent,
		            const ki::String* hist,   int histLen,
		            const ki::String* pinned, int pinnedLen,
		            ConfigManager& cfg)
			: DlgImpl(IDD_EXFILTER)
			, hist_(hist), histLen_(histLen)
			, pinned_(pinned), pinnedLen_(pinnedLen), pinnedCount_(0)
			, parent_(parent), cfg_(cfg)
		{ GoModal(parent_); }

		void SelectListItem(int idx) {
			HWND hList = item(IDC_FILTERCMDBOX);
			ListView_SetItemState(hList, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
			if (idx >= 0) {
				ListView_SetItemState(hList, idx, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				ListView_EnsureVisible(hList, idx, FALSE);
				// Update command box with selected command
				TCHAR cmd[2048]; cmd[0] = 0;
				ListView_GetItemText(hList, idx, 2, cmd, countof(cmd));
				::SetWindowText(item(IDC_FILTERCOMMANDBOX), cmd);
			}
		}

		void RebuildListView(int selectIdx = -1) {
			HWND hList = item(IDC_FILTERCMDBOX);
			ListView_DeleteAllItems(hList);
			pinnedCount_ = 0;

			// Insert pinned items with * marker
			// Columns: 0=dummy, 1=pin marker, 2=command
			for (int i = 0; i < pinnedLen_; ++i) {
				if (pinned_[i].len() == 0) break;
				LVITEM lvi;
				lvi.mask = LVIF_TEXT;
				lvi.iItem = i;
				lvi.iSubItem = 0;
				lvi.pszText = TEXT("");
				ListView_InsertItem(hList, &lvi);
				lvi.iSubItem = 1;
				lvi.pszText = TEXT("*");
				ListView_SetItem(hList, &lvi);
				lvi.iSubItem = 2;
				lvi.pszText = (TCHAR*)pinned_[i].c_str();
				ListView_SetItem(hList, &lvi);
				++pinnedCount_;
			}

			// Insert regular history items
			for (int i = 0; i < histLen_; ++i) {
				if (hist_[i].len() == 0) continue;
				LVITEM lvi;
				lvi.mask = LVIF_TEXT;
				lvi.iItem = pinnedCount_ + i;
				lvi.iSubItem = 0;
				lvi.pszText = TEXT("");
				ListView_InsertItem(hList, &lvi);
				lvi.iSubItem = 1;
				lvi.pszText = TEXT("");
				ListView_SetItem(hList, &lvi);
				lvi.iSubItem = 2;
				lvi.pszText = (TCHAR*)hist_[i].c_str();
				ListView_SetItem(hList, &lvi);
			}

			if (selectIdx >= 0)
				SelectListItem(selectIdx);
		}

		void UpdatePinBtn() {
			TCHAR cmd[2048]; cmd[0] = 0;
			GetItemText(IDC_FILTERCOMMANDBOX, countof(cmd), cmd);
			if (cmd[0]) {
				bool pinned = cfg_.IsFilterPinned(ki::String(cmd));
				const wchar_t* label;
				if (pinned) {
					label = LangManager::Get().GetDlgText(IDD_EXFILTER, L"IDC_FILTERPINBTN.unpin");
					if (!label) label = TEXT("Unpin");
				} else {
					label = LangManager::Get().GetDlgCtrlText(IDD_EXFILTER, IDC_FILTERPINBTN);
					if (!label) label = TEXT("Pin");
				}
				::SetWindowText(item(IDC_FILTERPINBTN), label);
			}
		}

		void on_init() override {
			LangManager::Get().ApplyToDialog(hwnd(), IDD_EXFILTER);
			SetCenter(hwnd(), parent_);

			HWND hList = item(IDC_FILTERCMDBOX);

			// Set extended list view style
			ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

			// Add columns with localized titles
			LVCOLUMN lvc;
			// Get column titles: try language file first, fall back to RC resources
			TCHAR pinnedTitle[32]; pinnedTitle[0] = 0;
			TCHAR cmdTitle[32]; cmdTitle[0] = 0;

			const wchar_t* fromLang = LangManager::Get().GetString(IDS_EXFILTER_PINNED);
			if (fromLang) {
				wcscpy(pinnedTitle, fromLang);
			} else {
				::LoadString(::GetModuleHandle(NULL), IDS_EXFILTER_PINNED, pinnedTitle, countof(pinnedTitle));
			}

			fromLang = LangManager::Get().GetString(IDS_EXFILTER_COMMAND);
			if (fromLang) {
				wcscpy(cmdTitle, fromLang);
			} else {
				::LoadString(::GetModuleHandle(NULL), IDS_EXFILTER_COMMAND, cmdTitle, countof(cmdTitle));
			}

			lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;

			// Dummy column at index 0 (workaround: ListView column 0 ignores fmt alignment)
			lvc.fmt = LVCFMT_LEFT;
			lvc.pszText = TEXT("");
			lvc.cx = 0;
			ListView_InsertColumn(hList, 0, &lvc);

			lvc.fmt = LVCFMT_CENTER;
			lvc.pszText = pinnedTitle;
			lvc.cx = 46;
			ListView_InsertColumn(hList, 1, &lvc);

			// Command column fills remaining listview width
			RECT listRect; ::GetClientRect(hList, &listRect);
			lvc.fmt = LVCFMT_LEFT;
			lvc.pszText = cmdTitle;
			lvc.cx = listRect.right - 46;
			ListView_InsertColumn(hList, 2, &lvc);

			// Populate list view
			RebuildListView(0);
			UpdatePinBtn();
			::SetFocus(item(IDC_FILTERCOMMANDBOX));
		}

		bool on_ok() override {
			TCHAR cmd[2048]; cmd[0] = 0;
			GetItemText(IDC_FILTERCOMMANDBOX, countof(cmd), cmd);
			cmd_ = cmd;
			return true;
		}

		bool on_message( UINT msg, WPARAM wp, LPARAM lp ) override {
			if (msg == WM_NOTIFY) {
				LPNMHDR pnmh = (LPNMHDR)lp;
				if (pnmh->idFrom == IDC_FILTERCMDBOX) {
					if (pnmh->code == LVN_ITEMCHANGED) {
						LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)pnmh;
						if (pnmlv->uChanged & LVIF_STATE) {
							if ((pnmlv->uNewState & LVIS_SELECTED) != 0) {
								TCHAR cmd[2048]; cmd[0] = 0;
								ListView_GetItemText(item(IDC_FILTERCMDBOX), pnmlv->iItem, 2, cmd, countof(cmd));
								::SetWindowText(item(IDC_FILTERCOMMANDBOX), cmd);
								UpdatePinBtn();
							}
						}
						return true;
					}
					if (pnmh->code == NM_CUSTOMDRAW) {
						LPNMLVCUSTOMDRAW pCD = (LPNMLVCUSTOMDRAW)lp;
						if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) {
							// Request post-paint notification to draw header separator
							::SetWindowLongPtr(hwnd(), DWLP_MSGRESULT, CDRF_NOTIFYPOSTPAINT);
							return true;
						}
						if (pCD->nmcd.dwDrawStage == CDDS_POSTPAINT) {
							// Draw a line below the header
							HWND hList = item(IDC_FILTERCMDBOX);
							HWND hHeader = ListView_GetHeader(hList);
							RECT hdrRect;
							::GetWindowRect(hHeader, &hdrRect);
							::MapWindowPoints(NULL, hList, (LPPOINT)&hdrRect, 2);
							HDC hdc = pCD->nmcd.hdc;
							HPEN hPen = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
							HPEN hOld = (HPEN)::SelectObject(hdc, hPen);
							::MoveToEx(hdc, hdrRect.left, hdrRect.bottom, NULL);
							::LineTo(hdc, hdrRect.right, hdrRect.bottom);
							::SelectObject(hdc, hOld);
							::DeleteObject(hPen);
							::SetWindowLongPtr(hwnd(), DWLP_MSGRESULT, CDRF_DODEFAULT);
							return true;
						}
					}
				}
			}
			return false;
		}

		bool on_command( UINT notif, UINT id, HWND ) override {
			if (id == IDC_FILTERCOMMANDBOX && notif == EN_CHANGE) {
				UpdatePinBtn();
				return false;
			}
			if (id == IDC_FILTERDELBTN) {
				int sel = (int)SendMsgToItem(IDC_FILTERCMDBOX, LVM_GETNEXTITEM, (WPARAM)-1, MAKELPARAM(LVNI_SELECTED, 0));
				if (sel == -1) return true;
				TCHAR cmd[2048]; cmd[0] = 0;
				ListView_GetItemText(item(IDC_FILTERCMDBOX), sel, 2, cmd, countof(cmd));
				if (sel < pinnedCount_) {
					cfg_.RemovePinned(ki::String(cmd));
				} else {
					cfg_.RemoveFilterHistory(ki::String(cmd));
				}
				RebuildListView(sel > 0 ? sel - 1 : 0);
				UpdatePinBtn();
				return true;
			}
			if (id == IDC_FILTERUPBTN || id == IDC_FILTERDOWNBTN) {
				int sel = (int)SendMsgToItem(IDC_FILTERCMDBOX, LVM_GETNEXTITEM, (WPARAM)-1, MAKELPARAM(LVNI_SELECTED, 0));
				int count = (int)SendMsgToItem(IDC_FILTERCMDBOX, LVM_GETITEMCOUNT, 0, 0);
				if (sel == -1) return true;
				int other = (id == IDC_FILTERUPBTN) ? sel - 1 : sel + 1;
				if (other < 0 || other >= count) return true;
				if ((sel < pinnedCount_) != (other < pinnedCount_)) return true;

				TCHAR aCmdBuf[2048], bCmdBuf[2048];
				ListView_GetItemText(item(IDC_FILTERCMDBOX), sel, 2, aCmdBuf, countof(aCmdBuf));
				ListView_GetItemText(item(IDC_FILTERCMDBOX), other, 2, bCmdBuf, countof(bCmdBuf));

				if (sel < pinnedCount_)
					cfg_.SwapPinned(sel, other);
				else
					cfg_.SwapFilterHistory(sel - pinnedCount_, other - pinnedCount_);

				RebuildListView(other);
				return true;
			}
			if (id == IDC_FILTERPINBTN) {
				int sel = (int)SendMsgToItem(IDC_FILTERCMDBOX, LVM_GETNEXTITEM, (WPARAM)-1, MAKELPARAM(LVNI_SELECTED, 0));
				if (sel == -1) return true;
				TCHAR cmd[2048]; cmd[0] = 0;
				ListView_GetItemText(item(IDC_FILTERCMDBOX), sel, 2, cmd, countof(cmd));
				if (cmd[0] == 0) return true;
				ki::String kcmd(cmd);

				if (sel < pinnedCount_) {
					cfg_.RemovePinned(kcmd);
					cfg_.AddFilterHistory(kcmd);
					RebuildListView(sel - 1 >= 0 ? sel - 1 : 0);
				} else {
					if (pinnedCount_ >= pinnedLen_) {
						TCHAR msg[256];
						const wchar_t* fmt = LangManager::Get().GetString(IDS_EXFILTER_PINFULL);
						if (!fmt) fmt = TEXT("Pinned list is full (max: %d).");
						wsprintf(msg, fmt, pinnedLen_);
						::MessageBox(hwnd(), msg, NULL, MB_OK | MB_ICONWARNING);
						return true;
					}
					cfg_.AddPinned(kcmd);
					RebuildListView(0);
				}
				return true;
			}
			return false;
		}

		const ki::String* hist_;
		int               histLen_;
		const ki::String* pinned_;
		int               pinnedLen_;
		int               pinnedCount_;
		HWND              parent_;
		ConfigManager&    cfg_;
		ki::String        cmd_;
	} dlg(hwnd(),
	      &cfg_.filterHistory(0), ConfigManager::kFilterHistoryMax,
	      &cfg_.filterPinned(0),  ConfigManager::kFilterPinnedMax,
	      cfg_);

	if (dlg.endcode() != IDOK || dlg.cmd_.len() == 0)
		return;

	const ki::String& userCmd = dlg.cmd_;

	// --- Prepare: save cursor, select target text ---
	const view::VPos *vCur, *vSel;
	bool hadSelection = edit_.getCursor().getCurPosUnordered(&vCur, &vSel);
	DPos origCurPos(vCur->tl, vCur->ad);

	if (!hadSelection) {
		// No selection: operate on whole file
		edit_.getCursor().Home(true, false);
		edit_.getCursor().End(true, true);
	}
	aarr<unicode> selText = edit_.getCursor().getSelectedStr();

	// --- Write selected text to temp input file ---
	TCHAR tmpDir[MAX_PATH], tmpIn[MAX_PATH];
	GetTempPath(MAX_PATH, tmpDir);
	if (GetTempFileName(tmpDir, TEXT("gpf"), 0, tmpIn) == 0) {
		if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);
		return;
	}

	{
		TextFileW tf(resolveCSI(csi_), lb_);
		if (!tf.Open(tmpIn)) {
			DeleteFile(tmpIn);
			if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);
			return;
		}
		const unicode* p = selText.get();
		const unicode* start = p;
		while (*p) {
			if (*p == L'\n') {
				size_t len = p - start;
				if (len > 0 && start[len-1] == L'\r') --len;
				tf.WriteLine(start, len, false);
				start = p + 1;
			}
			++p;
		}
		if (p > start)
			tf.WriteLine(start, p - start, true);
		tf.Close();
	}

	// --- Create pipes ---
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	HANDLE hStdinR, hStdinW, hStdoutR, hStdoutW, hStderrR, hStderrW;
	if (!CreatePipe(&hStdinR, &hStdinW, &sa, 0) ||
	    !CreatePipe(&hStdoutR, &hStdoutW, &sa, 0) ||
	    !CreatePipe(&hStderrR, &hStderrW, &sa, 0)) {
		DeleteFile(tmpIn);
		if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);
		return;
	}
	SetHandleInformation(hStdinW,  HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStdoutR, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStderrR, HANDLE_FLAG_INHERIT, 0);

	// --- Launch: cmd.exe /c <userCmd> ---
	String cmdLine = TEXT("cmd.exe /c ");
	cmdLine += userCmd;

	STARTUPINFO sti = { sizeof(STARTUPINFO) };
	sti.dwFlags    = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	sti.hStdInput  = hStdinR;
	sti.hStdOutput = hStdoutW;
	sti.hStdError  = hStderrW;
	sti.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION psi;
	bool launched = CreateExternalProcess(
		const_cast<TCHAR*>(cmdLine.c_str()),
		TRUE, CREATE_NO_WINDOW, &sti, &psi) != FALSE;

	CloseHandle(hStdinR);
	CloseHandle(hStdoutW);
	CloseHandle(hStderrW);

	if (!launched) {
		CloseHandle(hStdinW); CloseHandle(hStdoutR); CloseHandle(hStderrR);
		DeleteFile(tmpIn);
		if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);
		return;
	}
	CloseHandle(psi.hThread);

	// Add to history (process launched)
	cfg_.AddFilterHistory(userCmd);

	// --- Stdin writer thread ---
	HANDLE hTmpIn = CreateFile(tmpIn, GENERIC_READ, FILE_SHARE_READ,
	                           NULL, OPEN_EXISTING, 0, NULL);
	if (hTmpIn == INVALID_HANDLE_VALUE) {
		// Cannot open temp file; close remaining handles and terminate child
		CloseHandle(hStdinW);
		CloseHandle(hStdoutR);
		CloseHandle(hStderrR);
		TerminateProcess(psi.hProcess, 1);
		CloseHandle(psi.hProcess);
		DeleteFile(tmpIn);
		if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);
		return;
	}
	ExfStdinArgs stdinArgs = { hTmpIn, hStdinW };
	HANDLE hWriteThread = CreateThread(NULL, 0, ExfStdinThread, &stdinArgs, 0, NULL);

	// --- Stderr reader thread ---
	ExfStderrArgs stderrArgs;
	stderrArgs.hSrc = hStderrR;
	stderrArgs.size = 0;
	stderrArgs.cap  = 4096;
	stderrArgs.buf  = (BYTE*)HeapAlloc(GetProcessHeap(), 0, stderrArgs.cap);
	HANDLE hStderrThread = CreateThread(NULL, 0, ExfStderrThread, &stderrArgs, 0, NULL);

	// --- Main thread: read stdout ---
	DWORD stdoutCap  = 256 * 1024;
	BYTE* stdoutBuf  = (BYTE*)HeapAlloc(GetProcessHeap(), 0, stdoutCap);
	DWORD stdoutSize = 0;
	if (stdoutBuf) {
		BYTE tmp[8192]; DWORD n;
		while (ReadFile(hStdoutR, tmp, sizeof(tmp), &n, NULL) && n > 0) {
			if (stdoutSize + n > stdoutCap) {
				DWORD newCap = stdoutCap * 2 + n;
				BYTE* nb = (BYTE*)HeapReAlloc(GetProcessHeap(), 0, stdoutBuf, newCap);
				if (!nb) break;
				stdoutBuf = nb; stdoutCap = newCap;
			}
			memcpy(stdoutBuf + stdoutSize, tmp, n);
			stdoutSize += n;
		}
	}
	CloseHandle(hStdoutR);

	// --- Wait for everything ---
	WaitForSingleObject(hWriteThread, INFINITE);
	CloseHandle(hWriteThread);
	WaitForSingleObject(hStderrThread, INFINITE);
	CloseHandle(hStderrThread);
	WaitForSingleObject(psi.hProcess, INFINITE);
	DWORD exitCode = 1;
	GetExitCodeProcess(psi.hProcess, &exitCode);
	CloseHandle(psi.hProcess);

	// exit code 1 with no stderr = "no matches" (grep convention); treat as success
	bool noStderr = !stderrArgs.buf || stderrArgs.size == 0;
	bool treatAsSuccess = (exitCode == 0) || (exitCode == 1 && noStderr);

	if (treatAsSuccess && !stdoutBuf) {
		// stdout buffer allocation failed; leave text unchanged silently
		if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);
	} else if (treatAsSuccess) {
		// Decode stdout bytes directly from memory
		TextFileR tfr(resolveCSI(csi_));
		if (tfr.OpenFromMemory(stdoutBuf, stdoutSize)) {
			const size_t READ_CHUNK = 4096;
			size_t resCap = READ_CHUNK * 2;
			unicode* resBuf = (unicode*)HeapAlloc(GetProcessHeap(), 0, resCap * sizeof(unicode));
			size_t resLen = 0;
			if (resBuf) {
				size_t n;
				while ((n = tfr.ReadBuf(resBuf + resLen,
				        (resCap - resLen - 1) < READ_CHUNK ? resCap - resLen - 1 : READ_CHUNK)) > 0) {
					resLen += n;
					if (resLen + READ_CHUNK + 1 >= resCap) {
						size_t newCap = resCap * 2;
						unicode* nb = (unicode*)HeapReAlloc(GetProcessHeap(), 0,
						                                    resBuf, newCap * sizeof(unicode));
						if (!nb) break;
						resBuf = nb; resCap = newCap;
					}
				}
				resBuf[resLen] = 0;
				edit_.getCursor().Input(resBuf, resLen);
				HeapFree(GetProcessHeap(), 0, resBuf);
			}
		}
	} else {
		// Restore cursor when filter reported an error
		if (!hadSelection) edit_.getCursor().MoveCur(origCurPos, false);

		// Build error message
		String errMsg;
		const wchar_t* fmt = LangManager::Get().GetString(IDS_EXFILTER_FAILED);
		TCHAR header[512];
		if (fmt)
			wsprintf(header, fmt, (int)exitCode);
		else
			wsprintf(header, TEXT("External filter failed (exit code: %d)"), (int)exitCode);
		errMsg = header;

		// Append stderr (try UTF-8 first; fall back to OEM code page for cmd.exe messages)
		if (stderrArgs.buf && stderrArgs.size > 0) {
			UINT cp = CP_UTF8;
			int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			    (LPCSTR)stderrArgs.buf, (int)stderrArgs.size, NULL, 0);
			if (wlen == 0) {
				cp = CP_OEMCP;
				wlen = MultiByteToWideChar(CP_OEMCP, 0,
				    (LPCSTR)stderrArgs.buf, (int)stderrArgs.size, NULL, 0);
			}
			if (wlen > 0) {
				unicode* wbuf = new unicode[wlen + 1];
				if (wbuf) {
					MultiByteToWideChar(cp, 0,
					    (LPCSTR)stderrArgs.buf, (int)stderrArgs.size, wbuf, wlen);
					wbuf[wlen] = 0;
					errMsg += TEXT("\n");
					errMsg += wbuf;
					delete[] wbuf;
				}
			}
		}
		MsgBox(errMsg.c_str(), RzsString(IDS_APPNAME).c_str(), MB_OK | MB_ICONWARNING);
	}

	// Cleanup
	if (stdoutBuf)      HeapFree(GetProcessHeap(), 0, stdoutBuf);
	if (stderrArgs.buf) HeapFree(GetProcessHeap(), 0, stderrArgs.buf);
	DeleteFile(tmpIn);
}
