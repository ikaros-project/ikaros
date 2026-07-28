class WebUIWidgetSlider extends WebUIWidgetControl
{
    static template(header="SLIDER")
    {
        return [
            {name: header, control: "header"},
            {name: "title", default: "Sliders", type: "string", control: "textedit"},

            {name: "CONTROL", control: "header"},
            {name: "parameter", default: "", type: "source", control: "textedit"},
            {name: "command", default: "", type: "source", control: "textedit"},
            {name: "enabled_source", default: "", type: "source", control: "textedit"},
            {name: "select_x", default: 0, type: "int", control: "textedit"},
            {name: "select_y", default: "", type: "string", control: "textedit"},
            {name: "control_count", default: 1, type: "int", control: "textedit"},

            {name: "STYLE", control: "header"},
            {name: "labels", default: "", type: "string", control: "textedit"},
            {name: "min", default: 0, type: "string", control: "textedit"},
            {name: "max", default: 1, type: "string", control: "textedit"},
            {name: "step", default: 0.01, type: "float", control: "textedit"},
            {name: "show_values", default: "no", type: "bool", control: "checkbox"}
        ];
    }

    requestData(dataSet)
    {
        if(this.parameters.parameter)
            this.addSource(dataSet, this.parameters.parameter);
        if(this.parameters.enabled_source)
            this.addSource(dataSet, this.parameters.enabled_source);
    }

    disconnectedCallback()
    {
        if(typeof super.disconnectedCallback === "function")
            super.disconnectedCallback();
        if(this.keyDownHandler)
            document.removeEventListener("keydown", this.keyDownHandler);
        if(this.keyUpHandler)
            document.removeEventListener("keyup", this.keyUpHandler);
        this.keyDownHandler = null;
        this.keyUpHandler = null;
    }

    bindKeyHandlers()
    {
        if(this.keyDownHandler || this.keyUpHandler)
            return;
        this.keyDownHandler = (event) => {
            if(event.shiftKey)
                this.sync = true;
        };
        this.keyUpHandler = (event) => {
            if(!event.shiftKey)
                this.sync = false;
        };
        document.addEventListener("keydown", this.keyDownHandler);
        document.addEventListener("keyup", this.keyUpHandler);
    }

    getSliders()
    {
        return this.querySelectorAll("input");
    }

    syncEnabledState()
    {
        this.syncControlEnabledState(this.getSliders(), "div");
    }

    updateValueLabels()
    {
        const controls = this.firstChild?.children ?? [];
        for(const control of controls)
        {
            const value = control.querySelector(".slider_value");
            const slider = control.querySelector("input");
            if(value && slider)
                value.innerText = slider.value;
        }
    }

    sendSliderValue(value, index)
    {
        const x = this.getSelectX(index);
        const y = this.getSelectY();
        if(this.parameters.command)
        {
            this.send_command(this.parameters.command, 0, Number(value), y === "" ? x : y);
            return;
        }
        this.sendIndexedControlChange(this.parameters.parameter, value, index);
    }

    syncAllSliders(value)
    {
        const sliders = this.getSliders();
        const count = this.controlCount();
        for(let i = 0; i < count; i++)
        {
            this.sendSliderValue(value, i);
            if(sliders[i])
                sliders[i].value = value;
        }
        if(this.toBool(this.parameters.show_values))
            this.updateValueLabels();
    }

    numericParameterList(name, fallback)
    {
        const raw = this.parameters[name];
        const items = Array.isArray(raw) ? raw : String(raw ?? "").split(",");
        const values = items.map((item) => Number(String(item).trim())).filter(Number.isFinite);
        return values.length > 0 ? values : [fallback];
    }

    numericParameterValue(values, index)
    {
        return values[Math.min(index, values.length - 1)];
    }

    controlCount()
    {
        const count = Number(this.parameters.control_count);
        return Number.isFinite(count) ? Math.max(1, Math.trunc(count)) : 1;
    }

    sliderMoved(value, index=0, shiftPressed=false)
    {
        this.is_active = true;
        this.active_until = Date.now() + 500;
        if(this.controlCount() > 1 && (shiftPressed || this.sync))
            this.syncAllSliders(value);
        else
        {
            this.sendSliderValue(value, index);
            if(this.toBool(this.parameters.show_values))
                this.updateValueLabels();
        }
    }

    createSliderControl()
    {
        const control = document.createElement("div");
        const label = document.createElement("span");
        label.className = "slider_label";
        const input = document.createElement("input");
        input.type = "range";
        const value = document.createElement("span");
        value.className = "slider_value";
        value.innerText = "0";
        control.append(label, input, value);
        return control;
    }

    layoutSliders() {}

    updateAll()
    {
        super.updateAll();
        const container = this.firstChild;
        const count = this.controlCount();
        while(container.childElementCount > count)
            container.removeChild(container.lastElementChild);
        while(container.childElementCount < count)
            container.appendChild(this.createSliderControl());

        const sliders = this.getSliders();
        const labels = this.querySelectorAll(".slider_label");
        const values = this.querySelectorAll(".slider_value");
        const minimums = this.numericParameterList("min", 0);
        const maximums = this.numericParameterList("max", 1);
        const configuredStep = Number(this.parameters.step);
        const step = Number.isFinite(configuredStep) && configuredStep > 0 ? configuredStep : 0.01;
        sliders.forEach((slider, index) => {
            slider.min = this.numericParameterValue(minimums, index);
            slider.max = this.numericParameterValue(maximums, index);
            slider.step = step;
        });

        const rawLabels = String(this.parameters.labels ?? "").trim();
        const labelParts = rawLabels === "" ? [] : rawLabels.split(",").map((item) => item.trim());
        labels.forEach((label, index) => {
            label.style.display = labelParts.length > 0 ? "block" : "none";
            label.innerText = labelParts[index] ?? "";
        });
        for(const value of values)
            value.style.display = this.toBool(this.parameters.show_values) ? "block" : "none";

        this.layoutSliders();
        this.updateValueLabels();
        this.syncEnabledState();
        this.bindKeyHandlers();

        sliders.forEach((slider, index) => {
            slider.oninput = (event) => {
                if(!main.edit_mode)
                    this.sliderMoved(slider.value, index, event.shiftKey);
            };
            slider.onchange = () => {
                this.is_active = false;
                this.active_until = Date.now() + 500;
            };
            slider.onblur = slider.onchange;
            const stopPropagation = (event) => {
                this.is_active = false;
                this.active_until = main.edit_mode ? 0 : Date.now() + 500;
                if(!main.edit_mode)
                    event.stopPropagation();
            };
            slider.onmousedown = stopPropagation;
            slider.onmouseup = stopPropagation;
            slider.onclick = stopPropagation;
        });
    }

    update()
    {
        if(this.toBool(this.parameters.show_values))
            this.updateValueLabels();
        this.syncEnabledState();
        if(this.is_active || Date.now() < (this.active_until || 0))
            return;

        try
        {
            const data = this.getSource("parameter");
            if(data === undefined || data === null)
                return;
            if(this.getSelectY() !== "" && !(Array.isArray(data) && Array.isArray(data[0])))
                return;

            const sliders = this.getSliders();
            sliders.forEach((slider, index) => {
                const value = this.getSelectedSourceValue("parameter", undefined, index);
                if(value !== undefined)
                    slider.value = value;
            });
            if(this.toBool(this.parameters.show_values))
                this.updateValueLabels();
        }
        catch(err)
        {
        }
    }
}
