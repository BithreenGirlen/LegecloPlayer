#ifndef ADV_H_
#define ADV_H_

#include <string>

namespace adv
{
	struct TextDatum
	{
		std::wstring wstrText;
		std::wstring wstrVoicePath;
	};

	struct PaintDatum
	{
		bool isVideo = false;
		std::wstring wstrFilePath;
	};

	struct SceneDatum
	{
		size_t nTextIndex = 0;
		size_t nPaintIndex = 0;
	};

	struct LabelDatum
	{
		std::wstring wstrCaption;
		size_t nSceneIndex = 0;
	};
}
#endif // !ADV_H_
