#pragma once
#include "../EUINEO.h"
#include "../components/Panel.h"
#include "../components/Button.h"
#include "../components/ProgressBar.h"
#include "../components/Slider.h"
#include "../components/SegmentedControl.h"
#include "../components/Label.h"
#include "../components/InputBox.h"
#include "../components/ComboBox.h"

namespace EUINEO {

class MainPage {
public:
    Panel bgCircle1;
    Panel bgCircle2;
    Panel bgCircle3;
    Panel glassCard;

    Label titleLabel;
    Button btnPrimary;
    Button btnOutline;
    Button btnIcon;
    ProgressBar progBar;
    Slider slider;
    SegmentedControl segCtrl;
    InputBox inputBox;
    ComboBox comboBox;
    
    float testProgress = 0.0f;
    float testSliderVal = 50.0f;
    float previousGlassBlurAmount = -1.0f;
    Color previousGlassColor;
    bool hasPreviousGlassState = false;

    MainPage();
    void Update();
    void Draw();
};

} // namespace EUINEO
