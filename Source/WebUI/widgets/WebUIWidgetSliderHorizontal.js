class WebUIWidgetSliderHorizontal extends WebUIWidgetControl {
    static template() {
        return [
            { name: "SLIDER HORIZONTAL", control: "header" },
            { name: "title", default: "Sliders", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter", default: "", type: "source", control: "textedit" },
            { name: "command", default: "", type: "source", control: "textedit" },
            { name: "enabled_source", default: "", type: "source", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },
            { name: "control_count", default: 1, type: "int", control: "textedit" },

            { name: "STYLE", control: "header" },
            { name: "labels", default: "", type: "string", control: "textedit" },
            { name: "min", default: 0, type: "string", control: "textedit" },
            { name: "max", default: 1, type: "string", control: "textedit" },
            { name: "step", default: 0.01, type: "float", control: "textedit" },
            { name: "show_values", default: "no", type: "bool", control: "checkbox" }
        ];
    }

    static html() {
        return '<div class="hranger"></div>';
    }

    requestData(data_set) {
        if (this.parameters.parameter) {
            this.addSource(data_set, this.parameters.parameter);
        }
        if (this.parameters.enabled_source) {
            this.addSource(data_set, this.parameters.enabled_source);
        }
    }

    disconnectedCallback() {
        if (typeof super.disconnectedCallback === "function") {
            super.disconnectedCallback();
        }

        if (this._keyDownHandler) {
            document.removeEventListener("keydown", this._keyDownHandler);
            this._keyDownHandler = null;
        }

        if (this._keyUpHandler) {
            document.removeEventListener("keyup", this._keyUpHandler);
            this._keyUpHandler = null;
        }
    }

    _bindKeyHandlersOnce() {
        if (this._keyDownHandler || this._keyUpHandler) {
            return;
        }

        this._keyDownHandler = (event) => {
            if (event.shiftKey) {
                this.sync = true;
            }
        };

        this._keyUpHandler = (event) => {
            if (!event.shiftKey) {
                this.sync = false;
            }
        };

        document.addEventListener("keydown", this._keyDownHandler);
        document.addEventListener("keyup", this._keyUpHandler);
    }

    _getSliders() {
        return this.querySelectorAll("input");
    }

    _isEnabled() {
        if (!this.parameters.enabled_source) {
            return true;
        }

        const enabled_source = this.getSource("enabled_source", 1);
        const enableValue = Array.isArray(enabled_source)
            ? (Array.isArray(enabled_source[0]) ? enabled_source[0][0] : enabled_source[0])
            : enabled_source;
        return Number(enableValue) !== 0;
    }

    _syncEnabledState() {
        const enabled = this._isEnabled();
        const interactive = enabled && !main.edit_mode;
        this.classList.toggle("widget-control-disabled", !interactive);
        for (const slider of this._getSliders()) {
            slider.disabled = !interactive;
            slider.style.pointerEvents = main.edit_mode ? "none" : "";
            slider.closest("div")?.classList.toggle("widget-control-disabled", !interactive);
        }
    }

    _updateValueLabels() {
        const rows = this.firstChild?.children ?? [];
        for (const row of rows) {
            const value = row.querySelector(".slider_value");
            const slider = row.querySelector("input");
            if (value && slider) {
                value.innerText = slider.value;
            }
        }
    }

    _sendControlValue(value, index) {
        const configuredX = Number(this.parameters.select_x);
        const x = (Number.isFinite(configuredX) ? Math.max(0, Math.trunc(configuredX)) : 0) + index;
        const y = this.parameters.select_y;

        if (this.parameters.command) {
            const commandY = y === "" ? x : (Number.isFinite(Number(y)) ? Math.max(0, Math.trunc(Number(y))) : 0);
            this.send_command(
                this.parameters.command,
                0,
                Number(value),
                commandY
            );
            return;
        }

        if (!this.parameters.parameter) {
            return;
        }

        if (y === "") {
            this.send_control_change(this.parameters.parameter, value, x);
            return;
        }

        this.send_control_change(
            this.parameters.parameter,
            value,
            x,
            Number.isFinite(Number(y)) ? Math.max(0, Math.trunc(Number(y))) : 0
        );
    }

    _syncAllSliders(value) {
        const sliders = this._getSliders();
        const configuredCount = Number(this.parameters.control_count);
        const count = Number.isFinite(configuredCount) ? Math.max(1, Math.trunc(configuredCount)) : 1;

        for (let i = 0; i < count; i += 1) {
            this._sendControlValue(value, i);
            if (sliders[i]) {
                sliders[i].value = value;
            }
        }

        if (this.toBool(this.parameters.show_values)) {
            this._updateValueLabels();
        }
    }

    _getNumericParameterList(name, fallback) {
        const rawValue = this.parameters[name];
        const rawItems = Array.isArray(rawValue)
            ? rawValue
            : String(rawValue ?? "").split(",");
        const values = rawItems
            .map((item) => Number(String(item).trim()))
            .filter((value) => Number.isFinite(value));

        return values.length > 0 ? values : [fallback];
    }

    _getNumericParameterValue(values, index) {
        if (index < values.length) {
            return values[index];
        }
        return values[values.length - 1];
    }

    slider_moved(value, index = 0, shiftPressed = false) {
        this.is_active = true;
        this.active_until = Date.now() + 500;
        const shouldSync = Number(this.parameters.control_count) > 1 && (shiftPressed || this.sync);

        if (!shouldSync) {
            this._sendControlValue(value, index);
            if (this.toBool(this.parameters.show_values)) {
                this._updateValueLabels();
            }
            return;
        }

        this._syncAllSliders(value);
    }

    updateAll() {
        super.updateAll();

        const container = this.firstChild;
        const configuredCount = Number(this.parameters.control_count);
        const count = Number.isFinite(configuredCount) ? Math.max(1, Math.trunc(configuredCount)) : 1;

        while (container.childElementCount > count) {
            container.removeChild(container.lastElementChild);
        }

        while (container.childElementCount < count) {
            const row = document.createElement("div");

            const label = document.createElement("span");
            label.className = "slider_label";

            const input = document.createElement("input");
            input.type = "range";

            const value = document.createElement("span");
            value.className = "slider_value";
            value.innerText = "0";

            row.append(label, input, value);
            container.appendChild(row);
        }

        const sliders = this._getSliders();
        const labels = this.querySelectorAll(".slider_label");
        const values = this.querySelectorAll(".slider_value");

        const minValues = this._getNumericParameterList("min", 0);
        const maxValues = this._getNumericParameterList("max", 1);
        const configuredStep = Number(this.parameters.step);
        const step = Number.isFinite(configuredStep) && configuredStep > 0 ? configuredStep : 0.01;

        sliders.forEach((slider, index) => {
            slider.min = this._getNumericParameterValue(minValues, index);
            slider.max = this._getNumericParameterValue(maxValues, index);
            slider.step = step;
        });

        const rawLabels = String(this.parameters.labels ?? "").trim();
        const labelParts = rawLabels === "" ? [] : rawLabels.split(",").map((item) => item.trim());
        const showLabels = labelParts.length > 0;

        labels.forEach((label, index) => {
            label.style.display = showLabels ? "block" : "none";
            label.innerText = labelParts[index] ?? "";
        });

        for (const value of values) {
            value.style.display = this.toBool(this.parameters.show_values) ? "block" : "none";
        }

        this._updateValueLabels();
        this._syncEnabledState();

        this._bindKeyHandlersOnce();

        sliders.forEach((slider, index) => {
            slider.oninput = (event) => {
                if (main.edit_mode) {
                    return;
                }
                this.slider_moved(slider.value, index, event.shiftKey);
            };
            slider.onchange = () => {
                this.is_active = false;
                this.active_until = Date.now() + 500;
            };
            slider.onblur = slider.onchange;

            const stopWidgetPropagation = (event) => {
                if (main.edit_mode) {
                    this.is_active = false;
                    this.active_until = 0;
                    return;
                }
                this.is_active = false;
                this.active_until = Date.now() + 500;
                event.stopPropagation();
            };

            slider.onmousedown = stopWidgetPropagation;
            slider.onmouseup = stopWidgetPropagation;
            slider.onclick = stopWidgetPropagation;
        });
    }

    update() {
        if (this.toBool(this.parameters.show_values)) {
            this._updateValueLabels();
        }
        this._syncEnabledState();

        if (this.is_active || Date.now() < (this.active_until || 0)) {
            return;
        }

        try {
            let data = this.getSource("parameter");

            if (data === undefined || data === null) {
                return;
            }

            const sliders = this._getSliders();
            const isMatrix = Array.isArray(data) && Array.isArray(data[0]);

            if (this.parameters.select_y !== "") {
                if (!isMatrix) {
                    return;
                }

                const configuredY = Number(this.parameters.select_y);
                const selectedY = Number.isFinite(configuredY) ? Math.max(0, Math.trunc(configuredY)) : 0;
                const configuredX = Number(this.parameters.select_x);
                let x = Number.isFinite(configuredX) ? Math.max(0, Math.trunc(configuredX)) : 0;

                for (const slider of sliders) {
                    slider.value = data[selectedY]?.[x] ?? slider.value;
                    x += 1;
                }
                return;
            }

            const values = isMatrix ? data[0] : (Array.isArray(data) ? data : [data]);
            const configuredX = Number(this.parameters.select_x);
            let x = Number.isFinite(configuredX) ? Math.max(0, Math.trunc(configuredX)) : 0;
            for (const slider of sliders) {
                slider.value = values[x] ?? slider.value;
                x += 1;
            }

            if (this.toBool(this.parameters.show_values)) {
                this._updateValueLabels();
            }
        }
        catch (err) {
        }
    }
}

webui_widgets.add("webui-widget-slider-horizontal", WebUIWidgetSliderHorizontal);
