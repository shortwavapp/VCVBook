/*--------------------------- ABC ---------------------------------*
 *
 * Author: Leonardo Gabrielli <l.gabrielli@univpm.it>
 * License: GPLv3
 *
 * For a detailed guide of the code and functions see the book:
 * "Developing Virtual Synthesizers with VCV Rack" by L.Gabrielli
 *
 * Copyright 2020, Leonardo Gabrielli
 *
 *-----------------------------------------------------------------*/

#include "dsp/digital.hpp"
#include "AModal.hpp"

void AModal::process(const ProcessArgs &args) {

	bool changeValues = false;
	float fr = pow(params[PARAM_F0].getValue(), 10.0);
	if (inputs[VOCT_IN].isConnected()) {
		fr += dsp::FREQ_C4 * std::pow(2.f, 12.f * inputs[VOCT_IN].getVoltage() / 12.f);
	}
	if (f0 != fr) {
		f0 = fr;
		changeValues = true;
	}
	if (inhrm != params[PARAM_INHARM].getValue()) {
		inhrm = params[PARAM_INHARM].getValue();
		changeValues = true;
	}
	if (damp != params[PARAM_DAMP].getValue()) {
		damp = params[PARAM_DAMP].getValue();
		changeValues = true;
	}
	if (dsl != params[PARAM_DAMPSLOPE].getValue()) {
		dsl = params[PARAM_DAMPSLOPE].getValue();
		changeValues = true;
	}

	float mod_cv = params[PARAM_MOD_CV].getValue();

	if (changeValues) {
		for (int i = 0; i < MAX_OSC; i++) {
			float f = f0 * (float)(i+1);
			if ((i % 2) == 1)
				 f *= inhrm;
			float d = damp;
			if (dsl >= 0.0)
				d += (i * dsl);
			else
				d += ((MAX_OSC-i) * (-dsl));
			osc[i]->setCoeffs(f, d);
		}
	}

	float in = inputs[MAIN_IN].getVoltage();

	float invOut, cosOut, sinOut, cumOut = 0.0;
	for (int i = 0; i < nActiveOsc; i++) {
		osc[i]->process(in, &invOut, &cosOut, &sinOut);
		cumOut += sinOut;
	}

	if (inputs[MOD1_IN].isConnected())
		cumOut += cumOut * mod_cv * inputs[MOD1_IN].getVoltage();

	cumOut = cumOut / nActiveOsc;
	if (outputs[MAIN_OUT].isConnected())
		outputs[MAIN_OUT].setVoltage(cumOut);
}


struct AModalWidget : ModuleWidget {
	void appendContextMenu(Menu *menu) override;
	AModalWidget(AModal * module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
		box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		{
			ATitle * title = new ATitle(box.size.x);
			title->setText("AModal");
			addChild(title);
		}

		{
			ATextHeading * label = new ATextHeading(Vec(10, 30));
			label->setText("FREQUENCY");
			addChild(label);
		}

		{
			ATextHeading * label = new ATextHeading(Vec(18, 90));
			label->setText("DAMPING");
			addChild(label);
		}

		{
			ATextHeading * label = new ATextHeading(Vec(19, 150));
			label->setText("INHARM.");
			addChild(label);
		}

		{
			ATextHeading * label = new ATextHeading(Vec(5, 210));
			label->setText("SPEC.SLOPE");
			addChild(label);
		}

		{
			ATextLabel * label = new ATextLabel(Vec(17, 270));
			label->setText("IN");
			addChild(label);
		}

		{
			ATextLabel * label = new ATextLabel(Vec(15, 315));
			label->setText("MOD IN");
			addChild(label);
		}

		{
			ATextLabel * label = new ATextLabel(Vec(50, 270));
			label->setText("OUT");
			addChild(label);
		}

		addInput(createInput<PJ301MPort>(Vec(15, 345), module, AModal::MOD1_IN));
		addInput(createInput<PJ301MPort>(Vec(15, 300), module, AModal::MAIN_IN));
		addInput(createInput<PJ301MPort>(Vec(10, 70), module, AModal::VOCT_IN));

		addOutput(createOutput<PJ301MPort>(Vec(50, 300), module, AModal::MAIN_OUT));


		addParam(createParam<RoundBlackKnob>(Vec(50, 70), module, AModal::PARAM_F0));
		addParam(createParam<RoundBlackKnob>(Vec(30, 130), module, AModal::PARAM_DAMP));
		addParam(createParam<RoundBlackKnob>(Vec(30, 190), module, AModal::PARAM_INHARM));
		addParam(createParam<RoundBlackKnob>(Vec(30, 250), module, AModal::PARAM_DAMPSLOPE));
		addParam(createParam<Trimpot>(Vec(45, 350), module, AModal::PARAM_MOD_CV));

	}
};

void AModalWidget::appendContextMenu(Menu *menu) {
	AModal *module = dynamic_cast<AModal*>(this->module);

	menu->addChild(new MenuEntry);


	MenuLabel *modeLabel = new MenuLabel();
	modeLabel->text = "Oscillators";
	menu->addChild(modeLabel);

	nActiveOscMenuItem *nOsc1Item = new nActiveOscMenuItem();
	nOsc1Item->text = "1";
	nOsc1Item->module = module;
	nOsc1Item->nOsc = 1;
	nOsc1Item->rightText = CHECKMARK(module->nActiveOsc == nOsc1Item->nOsc);
	menu->addChild(nOsc1Item);

	nActiveOscMenuItem *nOsc16Item = new nActiveOscMenuItem();
	nOsc16Item->text = "16";
	nOsc16Item->module = module;
	nOsc16Item->nOsc = 16;
	nOsc16Item->rightText = CHECKMARK(module->nActiveOsc == nOsc16Item->nOsc);
	menu->addChild(nOsc16Item);

	nActiveOscMenuItem *nOsc32Item = new nActiveOscMenuItem();
	nOsc32Item->text = "32";
	nOsc32Item->module = module;
	nOsc32Item->nOsc = 32;
	nOsc32Item->rightText = CHECKMARK(module->nActiveOsc == nOsc32Item->nOsc);
	menu->addChild(nOsc32Item);

	nActiveOscMenuItem *nOsc64Item = new nActiveOscMenuItem();
	nOsc64Item->text = "64";
	nOsc64Item->module = module;
	nOsc64Item->nOsc = 64;
	nOsc64Item->rightText = CHECKMARK(module->nActiveOsc == nOsc64Item->nOsc);
	menu->addChild(nOsc64Item);

	/* additional spacer for future content
	MenuLabel *spacerLabel2 = new MenuLabel();
	menu->addChild(spacerLabel2);
	*/

}

json_t *AModal::dataToJson() {
	json_t *rootJ = json_object();
	json_object_set_new(rootJ, JSON_NOSC_KEY, json_integer(nActiveOsc));

	return rootJ;
}

void AModal::dataFromJson(json_t *rootJ) {
	json_t *nOscJ = json_object_get(rootJ, JSON_NOSC_KEY);
	if (nOscJ) {
		nActiveOsc = json_integer_value(nOscJ);
	}
}


Model *modelAModal = createModel<AModal, AModalWidget>("AModal");
