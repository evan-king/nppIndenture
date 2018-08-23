/*
*  evan-king/nppIndenture
*
*   Copyright 2017-2018 by Mike Tzou(Chocobo1)
*     https://github.com/Chocobo1/nppAutoDetectIndent
*
*   Copyright 2018 by Evan King(evan-king)
*     https://github.com/evan-king/nppIndenture
*
*   Licensed under GNU General Public License 3 or later.
*
*  @license GPL3 <https://www.gnu.org/licenses/gpl-3.0-standalone.html>
*/

#include "stdafx.h"
#include "nppIndenture.h"

#include <algorithm>
#include <stdexcept>

#include <Shellapi.h>
#include <Strsafe.h>

#include "settings.h"

#define PLUGIN_VERSION "1.0"

namespace {
	const int MAX_LINES = 5000;

	const int MIN_INDENT = 2; // minimum width of a single indentation
	const int MAX_INDENT = 8; // maximum width of a single indentation

	const int MIN_DEPTH = MIN_INDENT; // ignore lines below this indentation level
	const int MAX_DEPTH = 3*MAX_INDENT; // ignore lines beyond this indentation level
	
	// % of lines allowed to contradict indentation option without penalty
	const float GRACE_FREQUENCY = 1/50.0;

	struct ParseResult {
		int num_tab_lines = 0;
		int num_space_lines = 0;
		float grace = 0.0;
		
		// indentation => count(lines of that exact indentation)
		int depth_counts[MAX_DEPTH+1] = {0};
	};

	ParseResult parseDocument() {
		const auto sci = MyPlugin::instance()->message()->getSciCallFunctor();
		ParseResult result;

		const int num_lines = std::min(sci.call<int>(SCI_GETLINECOUNT), MAX_LINES);
		result.grace = float(num_lines) * GRACE_FREQUENCY;
		for(int i = 0; i < num_lines; ++i) {
			const int depth = sci.call<int>(SCI_GETLINEINDENTATION, i);
			if(depth < MIN_DEPTH || depth > MAX_DEPTH) continue;

			const int pos = sci.call<int>(SCI_POSITIONFROMLINE, i);
			const char lineHeadChar = sci.call<char>(SCI_GETCHARAT, pos);

			if(lineHeadChar == '\t') result.num_tab_lines++;

			if(lineHeadChar == ' ') {
				result.num_space_lines++;
				result.depth_counts[depth]++;
			}
		}

		return result;
	}
}

namespace MenuAction {
	void toggleMenuCheckbox(const int index, const Settings::BoolGetter getFunc, const Settings::BoolSetter setFunc) {
		const bool newSetting = !(Settings::instance()->*getFunc)();
		const MyPlugin *plugin = MyPlugin::instance();
		plugin->message()->sendNppMessage(NPPM_SETMENUITEMCHECK, plugin->m_funcItems[index]._cmdID, newSetting);
		(Settings::instance()->*setFunc)(newSetting);
	}

	void selectDisablePlugin() {
		toggleMenuCheckbox(0, &Settings::getDisablePlugin, &Settings::setDisablePlugin);

		MyPlugin *plugin = MyPlugin::instance();
		const bool isDisable = Settings::instance()->getDisablePlugin();
		if(isDisable) {
			plugin->indentCache.clear();
			nppIndenture::applyNppSettings(plugin->nppOriginalSettings);
		} else {
			const int id = plugin->message()->sendNppMessage<int>(NPPM_GETCURRENTBUFFERID, 0, 0);
			const nppIndenture::IndentInfo info = nppIndenture::detectIndentInfo();

			plugin->indentCache[id] = info;
			applyIndentInfo(info);
		}
	}

	void doNothing() {
		/*
		typedef std::chrono::high_resolution_clock Clock;
		auto t1 = Clock::now();
		// do something
		auto t2 = Clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
		*/
	}

	void gotoWebsite() {
		ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/evan-king/nppIndenture"), NULL, NULL, SW_SHOWDEFAULT);
	}
}

namespace nppIndenture {
	
	IndentInfo detectIndentInfo() {
		
		const ParseResult result = parseDocument();
		IndentInfo info;

		// decide `type`
		if(result.num_tab_lines + result.num_space_lines == 0)
			info.type = IndentInfo::IndentType::Invalid;
		else if(result.num_space_lines > (result.num_tab_lines * 4))
			info.type = IndentInfo::IndentType::Space;
		else if(result.num_tab_lines > (result.num_space_lines * 4))
			info.type = IndentInfo::IndentType::Tab;
		/*else
		{
			const auto sci = plugin.getSciCallFunctor();
			const bool useTab = sci.call<bool>(SCI_GETUSETABS);
			info.type = useTab ? IndentType::Tab : IndentType::Space;
		}*/

		// decide `num`
		if(info.type == IndentInfo::IndentType::Space) {
			
			// indent size => count(space-indented lines with incompatible indentation)
			int margins[MAX_INDENT+1] = {0};
			
			// for each depth option, count the incompatible lines
			for(int i = MIN_DEPTH; i <= MAX_DEPTH; i++) {
				for(int k = MIN_INDENT; k <= MAX_INDENT; k++) {
					if(i % k == 0) continue;
					margins[k] += result.depth_counts[i];
				}
			}

			// choose the last indent with the smallest margin (ties go to larger indent)
			// Considers margins within grace of zero as =zero,
			// so occasional typos don't force smaller indentation
			int which = MIN_INDENT;
			for(int i = MIN_INDENT; i <= MAX_INDENT; ++i) {
				if(result.depth_counts[i] == 0) continue;
				if(margins[i] <= margins[which] || margins[i] < result.grace) which = i;
			}

			info.num = which;
		}

		return info;
	}

	void applyIndentInfo(const IndentInfo &info) {
		const auto *message = MyPlugin::instance()->message();
		switch(info.type) {
			case IndentInfo::IndentType::Space: {
				message->postSciMessages({
					{SCI_SETTABINDENTS, true, 0},
					{SCI_SETINDENT, static_cast<WPARAM>(info.num), 0},
					{SCI_SETUSETABS, false, 0}
				});
				break;
			}

			case IndentInfo::IndentType::Tab: {
				message->postSciMessages({
					{SCI_SETTABINDENTS, false, 0},
					// no need of SCI_SETINDENT
					{SCI_SETUSETABS, true, 0}
				});
				break;
			}

			case IndentInfo::IndentType::Invalid: {
				break;  // do nothing
			}

			default:
				throw std::logic_error("Unexpected IndentInfo type");
		};
	}

	NppSettings detectNppSettings() {
		const auto sci = MyPlugin::instance()->message()->getSciCallFunctor();
		const bool tabIndents = sci.call<bool>(SCI_GETTABINDENTS);
		const bool useTabs = sci.call<bool>(SCI_GETUSETABS);
		const int indents = sci.call<int>(SCI_GETINDENT);
		return {tabIndents, useTabs, indents};
	}

	void applyNppSettings(const NppSettings &settings) {
		MyPlugin::instance()->message()->postSciMessages({
			{SCI_SETTABINDENTS, settings.tabIndents, 0},
			{SCI_SETINDENT, static_cast<WPARAM>(settings.indents), 0},
			{SCI_SETUSETABS, settings.useTabs, 0}
		});
	}
}


MyPlugin *MyPlugin::m_instance = nullptr;

MyPlugin::MyPlugin()
	: m_funcItems
	{{
		{TEXT("Disable plugin"), MenuAction::selectDisablePlugin},
		{TEXT("---"), nullptr},
		{TEXT("Version: " PLUGIN_VERSION), MenuAction::doNothing},
		{TEXT("Home page..."), MenuAction::gotoWebsite}
	}}
{
}

void MyPlugin::initInstance() {
	if(!m_instance)
		m_instance = new MyPlugin;
}

void MyPlugin::freeInstance() {
	delete m_instance;
	m_instance = nullptr;
}

MyPlugin* MyPlugin::instance() {
	return m_instance;
}

void MyPlugin::setupNppData(const NppData &data) {
	m_message = std::make_unique<Message>(data);
}

MyPlugin::Message* MyPlugin::message() const {
	return m_message.get();
}


MyPlugin::Message::Message(const NppData &data)
	: m_nppData(data) {}

void MyPlugin::Message::postSciMessages(const std::initializer_list<MessageParams> params) const {
	const HWND sciHwnd = getCurrentSciHwnd();
	for(const auto &param : params)
		::PostMessage(sciHwnd, param.msg, param.wParam, param.lParam);
}

MyPlugin::CallFunctor MyPlugin::Message::getSciCallFunctor() const {
	const HWND scintillaHwnd = getCurrentSciHwnd();

	static const SciFnDirect func = reinterpret_cast<SciFnDirect>(::SendMessage(scintillaHwnd, SCI_GETDIRECTFUNCTION, 0, 0));
	const sptr_t hnd = static_cast<sptr_t>(::SendMessage(scintillaHwnd, SCI_GETDIRECTPOINTER, 0, 0));
	return {func, hnd};
}

HWND MyPlugin::Message::getCurrentSciHwnd() const {
	int view = -1;
	::SendMessage(m_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&view));
	return (view == 0)
		? m_nppData._scintillaMainHandle
		: m_nppData._scintillaSecondHandle;
}
