class WebUIWidgetSliderVertical extends WebUIWidgetSlider
{
    static template()
    {
        return super.template("SLIDER VERTICAL");
    }

    static html()
    {
        return '<div class="vranger"></div>';
    }

    createSliderControl()
    {
        const control = document.createElement("div");
        const label = document.createElement("span");
        label.className = "slider_label";
        const slot = document.createElement("div");
        slot.className = "slider_slot";
        const input = document.createElement("input");
        input.type = "range";
        const value = document.createElement("span");
        value.className = "slider_value";
        value.innerText = "0";
        slot.appendChild(input);
        control.append(label, slot, value);
        return control;
    }

    layoutSliders()
    {
        const controls = this.firstChild?.children;
        if(!controls)
            return;
        for(const control of controls)
        {
            const label = control.querySelector(".slider_label");
            const slot = control.querySelector(".slider_slot");
            const slider = control.querySelector("input");
            const value = control.querySelector(".slider_value");
            if(!slider || !slot)
                continue;

            const labelHeight = label && getComputedStyle(label).display !== "none" ? label.getBoundingClientRect().height : 0;
            const valueHeight = value && getComputedStyle(value).display !== "none" ? value.getBoundingClientRect().height : 0;
            const slotStyle = getComputedStyle(slot);
            const padding = (parseFloat(slotStyle.paddingTop) || 0) + (parseFloat(slotStyle.paddingBottom) || 0);
            const slotHeight = slot.getBoundingClientRect().height;
            const fallbackHeight = control.getBoundingClientRect().height - labelHeight - valueHeight;
            slider.style.width = `${Math.max(24, (slotHeight || fallbackHeight) - padding)}px`;
        }
    }
}

webui_widgets.add("webui-widget-slider-vertical", WebUIWidgetSliderVertical);
