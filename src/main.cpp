#include <cutils/config_utils.h>
#include <cutils/misc.h>
#include <libgen.h>
#include <iostream>
#include <tinyxml2.h>
#include <vector>

struct library_t {
	std::string name;
	std::string path;
};

struct additionalEffect_t {
	std::string name;
	std::string library;
	std::string uuid;
};

struct effect_t {
	std::string name;
	std::string library;
	std::string uuid;
	std::vector<additionalEffect_t> additionalEffects;
};

struct postprocessEffect_t {
	std::string name;
	std::vector<std::string> effects;
};

struct preprocessEffect_t {
	std::string name;
	std::vector<std::string> effects;
};

void parseLibraries(cnode *root, std::vector<library_t> &libraries)
{
	auto librariesNode = config_find(root, "libraries");

	if (librariesNode == nullptr)
		return;

	for (auto libraryNode = librariesNode->first_child; libraryNode != nullptr; libraryNode = libraryNode->next)
	{
		auto pathNode = config_find(libraryNode, "path");

		if (pathNode != nullptr)
			libraries.push_back({libraryNode->name, basename(const_cast<char *>(pathNode->value))});
	}
}

void parseEffects(cnode *root, std::vector<effect_t> &effects)
{
	auto effectsNode = config_find(root, "effects");

	if (effectsNode == nullptr)
		return;

	for (auto effectNode = effectsNode->first_child; effectNode != nullptr; effectNode = effectNode->next)
	{
		auto libraryNode = config_find(effectNode, "library");

		if (libraryNode == nullptr)
			continue;

		auto uuidNode = config_find(effectNode, "uuid");

		if (uuidNode == nullptr)
			continue;

		auto parseAdditionalNode = [](cnode *root, std::vector<additionalEffect_t> &additionalEffects, const std::string &name)
		{
			auto localEffectNode = config_find(root, name.c_str());

			if (localEffectNode == nullptr)
				return;

			auto localLibraryNode = config_find(localEffectNode, "library");

			if (localLibraryNode == nullptr)
				return;

			auto localUuidNode = config_find(localEffectNode, "uuid");

			if (localUuidNode == nullptr)
				return;

			additionalEffects.push_back({localEffectNode->name, localLibraryNode->value, localUuidNode->value});
		};

		effect_t effect;
		effect.name = effectNode->name;
		effect.library = libraryNode->value;
		effect.uuid = uuidNode->value;

		for (auto additionalEffect = effectNode->first_child; additionalEffect != nullptr; additionalEffect = additionalEffect->next)
			parseAdditionalNode(effectNode, effect.additionalEffects, additionalEffect->name);

		effects.push_back(effect);
	}
}

void parsePostprocessEffects(cnode *root, std::vector<postprocessEffect_t> &postprocessEffects)
{
	auto postprocessingNode = config_find(root, "output_session_processing");

	if (postprocessingNode == nullptr)
		return;

	for (auto stream = postprocessingNode->first_child; stream != nullptr; stream = stream->next)
	{
		postprocessEffect_t postprocessEffect;
		postprocessEffect.name = stream->name;

		for (auto effect = stream->first_child; effect != nullptr; effect = effect->next)
			postprocessEffect.effects.emplace_back(effect->name);

		postprocessEffects.emplace_back(postprocessEffect);
	}
}

void parsePreprocessEffects(cnode *root, std::vector<preprocessEffect_t> &preprocessEffects)
{
	auto preprocessingNode = config_find(root, "pre_processing");

	if (preprocessingNode == nullptr)
		return;

	for (auto stream = preprocessingNode->first_child; stream != nullptr; stream = stream->next)
	{
		preprocessEffect_t preprocessEffect;
		preprocessEffect.name = stream->name;

		for (auto effect = stream->first_child; effect != nullptr; effect = effect->next)
			preprocessEffect.effects.emplace_back(effect->name);

		preprocessEffects.emplace_back(preprocessEffect);
	}
}

void writeAudioEffectsXml(const std::string &filePath, const std::vector<library_t> &libraries, const std::vector<effect_t> &effects, const std::vector<postprocessEffect_t> &postprocessEffects, const std::vector<preprocessEffect_t> &preprocessEffects)
{
	tinyxml2::XMLDocument xmlDocument;

	// Prepare declaration
	auto declaration = xmlDocument.NewDeclaration();
	declaration->SetValue(R"(xml version="1.0" encoding="UTF-8")");
	xmlDocument.InsertEndChild(declaration);

	// Prepare audio_effects_conf node
	auto audioEffectsConfElement = xmlDocument.NewElement("audio_effects_conf");
	audioEffectsConfElement->SetAttribute("version", "2.0");
	audioEffectsConfElement->SetAttribute("xmlns", "http://schemas.android.com/audio/audio_effects_conf/v2_0");

	// Prepare audio_effects_conf -> libraries node
	auto librariesElement = xmlDocument.NewElement("libraries");

	for (auto &library : libraries)
	{
		auto libraryElement = xmlDocument.NewElement("library");
		libraryElement->SetAttribute("name", library.name.c_str());
		libraryElement->SetAttribute("path", library.path.c_str());

		librariesElement->InsertEndChild(libraryElement);
	}

	if (librariesElement->FirstChildElement() != nullptr)
		audioEffectsConfElement->InsertEndChild(librariesElement);

	// Prepare audio_effects_conf -> effects node
	auto effectsElement = xmlDocument.NewElement("effects");

	for (auto &effect : effects)
	{
		auto effectElement = xmlDocument.NewElement(effect.library == "proxy" ? "effectProxy" : "effect");
		effectElement->SetAttribute("name", effect.name.c_str());
		effectElement->SetAttribute("library", effect.library.c_str());
		effectElement->SetAttribute("uuid", effect.uuid.c_str());

		for (auto &additionalEffect : effect.additionalEffects)
		{
			auto additionalEffectElement = xmlDocument.NewElement(additionalEffect.name.c_str());
			additionalEffectElement->SetAttribute("library", additionalEffect.library.c_str());
			additionalEffectElement->SetAttribute("uuid", additionalEffect.uuid.c_str());

			effectElement->InsertEndChild(additionalEffectElement);
		}

		effectsElement->InsertEndChild(effectElement);
	}

	if (effectsElement->FirstChildElement() != nullptr)
		audioEffectsConfElement->InsertEndChild(effectsElement);

	// Prepare audio_effects_conf -> postprocess node
	auto postprocessEffectsElement = xmlDocument.NewElement("postprocess");

	for (auto &postprocessEffect : postprocessEffects)
	{
		auto postprocessEffectElement = xmlDocument.NewElement("stream");
		postprocessEffectElement->SetAttribute("type", postprocessEffect.name.c_str());

		for (auto &effect : postprocessEffect.effects)
		{
			auto applypostprocessEffectElement = xmlDocument.NewElement("apply");
			applypostprocessEffectElement->SetAttribute("effect", effect.c_str());

			postprocessEffectElement->InsertEndChild(applypostprocessEffectElement);
		}

		postprocessEffectsElement->InsertEndChild(postprocessEffectElement);
	}

	if (postprocessEffectsElement->FirstChildElement() != nullptr)
		audioEffectsConfElement->InsertEndChild(postprocessEffectsElement);

	// Prepare audio_effects_conf -> preprocess node
	auto preprocessEffectsElement = xmlDocument.NewElement("preprocess");

	for (auto &preprocessEffect : preprocessEffects)
	{
		auto preprocessEffectElement = xmlDocument.NewElement("stream");
		preprocessEffectElement->SetAttribute("type", preprocessEffect.name.c_str());

		for (auto &effect : preprocessEffect.effects)
		{
			auto applyPreprocessEffectElement = xmlDocument.NewElement("apply");
			applyPreprocessEffectElement->SetAttribute("effect", effect.c_str());

			preprocessEffectElement->InsertEndChild(applyPreprocessEffectElement);
		}

		preprocessEffectsElement->InsertEndChild(preprocessEffectElement);
	}

	if (preprocessEffectsElement->FirstChildElement() != nullptr)
		audioEffectsConfElement->InsertEndChild(preprocessEffectsElement);

	// Save
	xmlDocument.InsertEndChild(audioEffectsConfElement);
	xmlDocument.SaveFile(filePath.c_str());
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cout << "Usage: ./" << basename(argv[0]) << " /path/to/audio_effects.conf /path/to/audio_effects.xml" << std::endl;
		return 1;
	}

	auto data = static_cast<char *>(load_file(argv[1], nullptr));

	if (data == nullptr)
		return 1;

	auto root = config_node("", "");
	config_load(root, data);

	// Libraries
	std::vector<library_t> libraries;
	parseLibraries(root, libraries);

	// Effects
	std::vector<effect_t> effects;
	parseEffects(root, effects);

	// Postprocess
	std::vector<postprocessEffect_t> postprocessEffects;
	parsePostprocessEffects(root, postprocessEffects);

	// Preprocess
	std::vector<preprocessEffect_t> preprocessEffects;
	parsePreprocessEffects(root, preprocessEffects);

	// Convert to XML
	writeAudioEffectsXml(argv[2], libraries, effects, postprocessEffects, preprocessEffects);

	return 0;
}
