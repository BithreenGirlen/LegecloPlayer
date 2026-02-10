#ifndef LEGECLO_H_
#define LEGECLO_H_

#include <string>
#include <vector>

#include "adv.h"

namespace legeclo
{
	bool LoadScenario(
		const std::wstring& wstrFilePath,
		std::vector<adv::TextDatum>& textData,
		std::vector<adv::PaintDatum>& paintData,
		std::vector<adv::SceneDatum>& sceneData,
		std::vector<adv::LabelDatum>& labelData
	);
}
#endif // !LEGECLO_H_
