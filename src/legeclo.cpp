

#if defined(_WIN32) && defined (_DEBUG)
#include <Windows.h> /* ::OutputDebugStringW */
#endif

#include <string_view>
#include <algorithm>

#include "legeclo.h"

#include "win_filesystem.h"
#include "win_text.h"

/* 内部用 */
namespace legeclo
{
	/* Taken from Adventure.AdvUserInstructionNo in Assembly-CSharp.dll */
	enum class EAdvUserInstructionNo
	{
		NOP = 30,
		READ_MESSAGE = 31,
		WAIT_WORK,
		OPEN_EPISODE,
		CLOSE_EPISODE,
		CHARA_DELETE,
		CHARA_DISP_REGISTRATION,
		CHARA_ACTION_JUMP,
		FILE_PRELOAD,
		CHARA_MOVE,
		DISP_EXECUTION,
		CHARA_OUT,
		CHARA_SLIDE,
		WHITE_IN,
		WHITE_OUT,
		BLACK_IN,
		BLACK_OUT,
		SHAKE,
		CAMERA_ZOOM_RESET,
		CAMERA_ZOOM,
		SE_STOP,
		SE_START,
		BGM_STOP,
		BGM_START,
		BG_DISP,
		BRANCH_DISP,
		BRANCH_RESULT,
		MESSAGE_WINDOW_DISP_SWITCH,
		EMOTION_SWITCH,
		MOVIE_PLAY,
		TALK_BRIGHTNESS,
		COLOR_IN_H,
		COLOR_OUT_H,
		FILE_LOAD,
		CHARA_EFFECT_ANIMATION,
		STOP_MOVIE,
		CHARA_MASK,
		BLACK_FRAME_ON,
		BLACK_FRAME_OFF,
		DARK_EFFECT_ON,
		DARK_EFFECT_OFF,
		STILL_DELETE,
		STILL_DISP_REGISTRATION,
		STILL_OUT,
		SUSPEND,
		EFFECT_ANIMATION_STOP,
		EFFECT_ANIMATION_DELETE,
		GRAY_SCALE,
		VOICE_STOP,
		VOICE_START,
		MESSAGE_WINDOW_TYPE,
		MESSAGE_WINDOW_TYPE_RESET,
		ALTER_KOMAGEKI,
		ALTER_CHANGE_CHARA_ATTACHMENT,
		MOVIE_PAUSE,
		MOVIE_RESUME,
		CHARA_FACE_PATTERN,
		EFFECT_ANIMATION_COORDINATE,
		PREFAB_PLAY,
		PREFAB_STOP,
		COLOR_IN,
		COLOR_OUT,
		COUNT_MAX
	};
	/// @brief Minimum command types to play event scenario
	enum class ECommandType
	{
		Unknown = -1,
		Message,
		Still,
		Video
	};

	struct ScriptCommand
	{
		ECommandType commandType = ECommandType::Unknown;
		size_t index = 0;
	};

	struct MessageDatum
	{
		std::string_view characterName;
		std::string_view message;

		std::string_view voiceFilename;
		std::string_view voiceTrackName;
	};
	
	struct StillDatum
	{
		std::string_view fileName;
	};

	struct MovieDatum
	{
		std::string_view fileName;
		bool loop = true;
	};

	struct ScriptData
	{
		std::vector<ScriptCommand> scriptCommands;

		std::vector<MessageDatum> messageData;
		std::vector<StillDatum> stillData;
		std::vector<MovieDatum> movieData;
	};

	static std::wstring g_wstrStillFolderPath;
	static std::wstring g_wstrVideoFolderPath;
	static std::wstring g_wstrVoiceFolderPath;

	static uint32_t ToUInt32(const char* src)
	{
		const uint8_t* p = reinterpret_cast<const uint8_t*>(src);
		return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	}

	/* 脚本ファイル読み取り */
	static void ReadScript(const std::string& strFile, ScriptData& scriptData)
	{
		static constexpr size_t kCommandOffset = 0x08;
		static constexpr size_t kCommandCount = 0x0c;
		static constexpr size_t kTextOffset = 0x10;
		static constexpr size_t kTextLength = 0x14;

		if (strFile.size() < kTextLength + 4ULL)return;

		uint32_t ulCommandPos = ToUInt32(&strFile[kCommandOffset]);
		uint32_t ulCommandCount = ToUInt32(&strFile[kCommandCount]);
		uint32_t ulTextPos = ToUInt32(&strFile[kTextOffset]);
		uint32_t ulTextLength = ToUInt32(&strFile[kTextLength]);

		size_t nTextEndPos = static_cast<size_t>(ulTextPos + ulTextLength);
		if (strFile.size() < nTextEndPos)return;
		size_t nCommandEndPos = ulCommandPos + (ulCommandCount * 8ULL);
		if (strFile.size() < nCommandEndPos)return;

		struct CommandArg
		{
			uint8_t type;
			uint32_t value;
		};

		struct CommandDatum
		{
			uint8_t type;
			union Datum
			{
				struct Params
				{
					uint16_t param1;
					uint16_t param2;
				};
				Params params;
				uint32_t value;
			};
			Datum datum;
			std::vector<CommandArg> args;
		};

		std::vector<CommandDatum> commandData;
		for (size_t nRead = ulCommandPos; nRead < nCommandEndPos;)
		{
			CommandDatum c;
			c.type = strFile[nRead];
			c.datum.value = ToUInt32(&strFile[nRead + 4]);
			nRead += 8ULL;

			if (c.type == 0)
			{
				const uint16_t argCount = c.datum.params.param2;
				if (nRead + (argCount * 8ULL) <= nCommandEndPos)
				{
					c.args.resize(argCount);
					for (uint16_t i = 0; i < argCount; ++i)
					{
						c.args[i].type = strFile[nRead];
						c.args[i].value = ToUInt32(&strFile[nRead + 4]);
						nRead += 8ULL;
					}
				}
			}

			commandData.push_back(std::move(c));
		}

		/* Debug output for commands specifying text offset. */
#if defined(_WIN32) && defined (_DEBUG)
		for (const auto& c : commandData)
		{
			const auto HasTextField = [&c]()
				{
					return std::any_of(c.args.begin(), c.args.end(), [](const auto& arg) { return arg.type == 3; });
				};

			if (HasTextField())
			{
				wchar_t sBuffer[1024]{};
				static constexpr size_t bufferSize = sizeof(sBuffer) / sizeof(wchar_t) - 1;

				int iLen = 0;
				iLen += swprintf_s(sBuffer + iLen, bufferSize - iLen, L"size: %zu, ", c.args.size());
				iLen += swprintf_s(sBuffer + iLen, bufferSize - iLen, L"type: %u, ", c.datum.params.param1);
				for (const auto& arg : c.args)
				{
					if (arg.type == 3)
					{
						std::wstring wstr = win_text::WidenUtf8(&strFile[ulTextPos + arg.value]);
						iLen += swprintf_s(sBuffer + iLen, bufferSize - iLen, L"s: %s, ", wstr.c_str());
					}
					else
					{
						iLen += swprintf_s(sBuffer + iLen, bufferSize - iLen, L"v: %u, ", arg.value);
					}
				}
				wcscat_s(sBuffer, L"\n");
				::OutputDebugStringW(sBuffer);
			}
		}
#endif /* End of debug output */

		const auto ToStringView = [&](uint32_t nStart)
			->std::string_view
			{
				if (nStart >= nTextEndPos)return {};

				size_t nPos = strFile.find('\0', nStart);
				if (nPos == std::string::npos)return {};

				return { &strFile[nStart] , nPos - nStart };
			};

		for (const auto& c : commandData)
		{
			const EAdvUserInstructionNo type = static_cast<EAdvUserInstructionNo>(c.datum.params.param1);
			if (type == EAdvUserInstructionNo::READ_MESSAGE)
			{
				/*
				* [0] 話者名開始位置,
				* [1] 台詞もしくは地の文開始位置,
				* [2] 23固定,
				* [3] 0固定,
				* [4] acbファイル名; 割り当て無しの場合空文字列,
				* [5] acbトラック名; 〃
				*/
				if (c.args.size() < 5)continue;

				MessageDatum s
				{
					.characterName = ToStringView(ulTextPos + c.args[0].value),
					.message = ToStringView(ulTextPos + c.args[1].value),
					.voiceFilename = ToStringView(ulTextPos + c.args[4].value),
					.voiceTrackName = ToStringView(ulTextPos + c.args[5].value)
				};

				scriptData.messageData.push_back(std::move(s));
				scriptData.scriptCommands.emplace_back(ScriptCommand{ ECommandType::Message, scriptData.messageData.size() - 1 });
			}
			else if (type == EAdvUserInstructionNo::MOVIE_PLAY)
			{
				/*
				* [0] ファイル名開始位置
				* [1] ループ指定; 0: 無し, 1: 有り
				*/
				if (c.args.size() < 2)continue;
				MovieDatum s{ .fileName = ToStringView(ulTextPos + c.args[0].value), .loop = c.args[1].value == 1};

				scriptData.movieData.push_back(std::move(s));
				scriptData.scriptCommands.emplace_back(ScriptCommand{ ECommandType::Video, scriptData.movieData.size() - 1 });
			}
			else if (type == EAdvUserInstructionNo::STILL_DISP_REGISTRATION)
			{
				/*
				* [0] 連番？
				* [1] ファイル名開始位置
				* [2] 5固定
				*/
				if (c.args.size() < 3)continue;
				StillDatum s{ .fileName = ToStringView(ulTextPos + c.args[1].value) };

				scriptData.stillData.push_back(std::move(s));
				scriptData.scriptCommands.emplace_back(ScriptCommand{ ECommandType::Still, scriptData.stillData.size() - 1 });
			}
		}
	}

	/* 脚本ファイル経路から各種資源階層導出 */
	static bool DeriveResourceFolderPathsFromScriptFilePath(const std::wstring& wstrFilePath)
	{
		size_t nGameDataPos = wstrFilePath.rfind(L"adv\\");
		if (nGameDataPos == std::wstring::npos)return false;

		size_t nAdvPos = wstrFilePath.rfind(L"scenario\\");
		if (nAdvPos == std::wstring::npos)return false;

		g_wstrStillFolderPath.assign(&wstrFilePath[0], nAdvPos).append(LR"(still\)");
		g_wstrVideoFolderPath.assign(&wstrFilePath[0], nAdvPos).append(LR"(movie\)");
		g_wstrVoiceFolderPath.assign(&wstrFilePath[0], nGameDataPos).append(LR"(sound\webgl\adv\)");

		return !g_wstrStillFolderPath.empty() && !g_wstrVideoFolderPath.empty() && !g_wstrVoiceFolderPath.empty();
	}

	static void ExtractFileNameWithoutExtension(const std::wstring& filePath, std::wstring& fileName)
	{
		size_t nPos1 = filePath.find_last_of(L"\\/");
		if (nPos1 == std::wstring::npos)nPos1 = 0;
		else ++nPos1;

		size_t nPos2 = filePath.find(L'.', nPos1);
		if (nPos2 == std::wstring::npos)nPos2 = filePath.size();

		fileName.assign(&filePath[nPos1], &filePath[nPos2]);
	}
} /* namespace legeclo */

bool legeclo::LoadScenario(const std::wstring& wstrFilePath, std::vector<adv::TextDatum>& textData, std::vector<adv::PaintDatum>& paintData, std::vector<adv::SceneDatum>& sceneData, std::vector<adv::LabelDatum>& labelData)
{
	/* 初期作成 */
	if (g_wstrStillFolderPath.empty())
	{
		if (!DeriveResourceFolderPathsFromScriptFilePath(wstrFilePath))return false;
	}

	/* 文章・音声・静画・動画に関する指令文の抜粋 */
	std::string strFile = win_filesystem::LoadFileAsString(wstrFilePath.c_str());
	if (strFile.empty())return false;

	ScriptData scriptData;
	ReadScript(strFile, scriptData);
	if (scriptData.scriptCommands.empty())return false;

	std::wstring labelCaptionBuffer; /* 画像・動画切り替わり場面 */
	size_t lastPaintIndex = 0; /* ループ内での空判断を避ける。 */

	for (const auto& scriptCommand : scriptData.scriptCommands)
	{
		/* 動的確保を避ける。 */
		wchar_t sBuffer[1024]{};
		static constexpr int bufferSize = sizeof(sBuffer) / sizeof(sBuffer[0]) - 1;
		int bufferLength = 0;

		const auto ToUtf16 = [&sBuffer, &bufferLength](std::string_view s)
			-> const wchar_t*
			{
				int iLength = win_text::WidenUtf8Static(s.data(), static_cast<int>(s.size()), sBuffer, bufferSize);
				if (iLength > 0) [[likely]] sBuffer[iLength] = L'\0';
				else [[unlikely]] wmemset(sBuffer, L'\0', bufferSize);

				bufferLength = iLength;

				return sBuffer;
			};

		const auto ReplaceStatic = [&sBuffer, &bufferLength](std::wstring_view strOld, std::wstring_view strNew)
			{
				for (size_t nLast = 0;;)
				{
					std::wstring_view s(sBuffer, bufferLength);
					size_t nPos = s.find(strOld, nLast);
					if (nPos == std::wstring_view::npos)break;

					wchar_t* pPos = sBuffer + nPos;
					wmemmove(pPos + strNew.size(), pPos + strOld.size(), bufferLength - nPos - strOld.size() + 1);
					wmemcpy(pPos, strNew.data(), strNew.size());

					bufferLength += static_cast<int>(strNew.size() - strOld.size());
					nLast = nPos + strNew.size();
				}
			};

		const auto EliminateTagStatic = [&sBuffer, &bufferLength]()
			{
				int nWritten = 0;
				int iCount = 0;
				for (int nRead = 0; nRead < bufferLength; ++nRead)
				{
					const wchar_t c = sBuffer[nRead];
					if (c == L'<')
					{
						++iCount;
						continue;
					}
					else if (c == L'>')
					{
						--iCount;
						continue;
					}

					if (iCount == 0)
					{
						sBuffer[nWritten] = c;
						++nWritten;
					}
				}

				bufferLength = nWritten;
				sBuffer[bufferLength] = L'\0';
			};

		size_t index = scriptCommand.index;
		if (scriptCommand.commandType == ECommandType::Message)
		{
			if (index < scriptData.messageData.size())
			{
				adv::TextDatum textDatum;

				const auto& messageDatum = scriptData.messageData[index];
				if (!messageDatum.characterName.empty())
				{
					textDatum.wstrText += ToUtf16(messageDatum.characterName);
					textDatum.wstrText += L':';
				}

				textDatum.wstrText += L" \n";
				/* Replace string in static buffer. */
				ToUtf16(messageDatum.message);
				ReplaceStatic(L"<name></name>", L"隊長");
				ReplaceStatic(L"$n", L"\n");
				EliminateTagStatic();
				textDatum.wstrText += sBuffer;

				if (!messageDatum.voiceFilename.empty() && !messageDatum.voiceTrackName.empty())
				{
					textDatum.wstrVoicePath += g_wstrVoiceFolderPath;
					textDatum.wstrVoicePath += ToUtf16(messageDatum.voiceFilename);
					textDatum.wstrVoicePath += L'\\';
					textDatum.wstrVoicePath += ToUtf16(messageDatum.voiceTrackName);
					textDatum.wstrVoicePath += L".wav";
				}

				textData.push_back(std::move(textDatum));
				sceneData.push_back(adv::SceneDatum{ textData.size() - 1, lastPaintIndex });
				if (!labelCaptionBuffer.empty())
				{
					labelData.emplace_back(adv::LabelDatum{ labelCaptionBuffer, sceneData.size() - 1 });
					labelCaptionBuffer.clear();
				}
			}
		}
		else if (scriptCommand.commandType == ECommandType::Still)
		{
			if (index < scriptData.stillData.size())
			{
				/* 最終動画と最終静画の間に文章が存在しない。  */
				if (!labelCaptionBuffer.empty())
				{
					sceneData.back().nPaintIndex = lastPaintIndex;
					labelData.emplace_back(adv::LabelDatum{ labelCaptionBuffer, sceneData.size() - 1 });
					labelCaptionBuffer.clear();
				}

				const auto& stillDatum = scriptData.stillData[index];

				adv::PaintDatum paintDatum
				{
					.isVideo = false,
					.wstrFilePath = g_wstrStillFolderPath + ToUtf16(stillDatum.fileName) + L".png"
				};

				ExtractFileNameWithoutExtension(paintDatum.wstrFilePath, labelCaptionBuffer);
				paintData.push_back(std::move(paintDatum));
				lastPaintIndex = paintData.size() - 1;
			}
		}
		else if (scriptCommand.commandType == ECommandType::Video)
		{
			if (index < scriptData.movieData.size())
			{
				const auto& movieDatum = scriptData.movieData[index];

				adv::PaintDatum paintDatum
				{
					.isVideo = true,
					.wstrFilePath = g_wstrVideoFolderPath + ToUtf16(movieDatum.fileName) + L".mp4"
				};

				ExtractFileNameWithoutExtension(paintDatum.wstrFilePath, labelCaptionBuffer);

				paintData.push_back(std::move(paintDatum));
				lastPaintIndex = paintData.size() - 1;
			}
		}
	}

	return !textData.empty() && !paintData.empty() && !sceneData.empty();
}
