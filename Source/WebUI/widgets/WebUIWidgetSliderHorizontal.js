class WebUIWidgetSliderHorizontal extends WebUIWidgetControl {
    static template() {
        return [
            { name: "SLIDER HORIZONTAL", control: "header" },
            { name: "title", default: "Sliders", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter", default: "", type: "source", control: "textedit" },
            { name: "enableSource", default: "", type: "source", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },
            { name: "count", default: 1, type: "int", control: "textedit" },

            { name: "STYLE", control: "header" },
            { name: "labels", default: "", type: "string", control: "textedit" },
            { name: "min", default: 0, type: "string", control: "textedit" },
            { name: "max", default: 1, type: "string", control: "textedit" },
            { name: "step", default: 0.01, type: "float", control: "textedit" },
            { name: "show_values", default: false, type: "bool", control: "checkbox" }
        ];
    }

    static html() {
        return '<div class="hranger"></div>';
    }

    requestData(data_set) {
        if (this.parameters.parameter) {
            this.addSource(data_set, this.parameters.parameter);
        }
        if (this.parameters.enableSource) {
            this.addSource(data_set, this.parameters.enableSource);
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
        if (!this.parameters.enableSource) {
            return true;
        }

        const enableSource = this.getSource("enableSource", 1);
        const enableValue = Array.isArray(enableSource)
            ? (Array.isArray(enableSource[0]) ? enableSource[0][0] : enableSource[0])
            : enableSource;
        return Number(enableValue) !== 0;
    }

    _syncEnabledState() {
        const enabled = this._isEnabled();
        this.classList.toggle("widget-control-disabled", !enabled);
        for (const slider of this._getSliders()) {
            slider.disabled = !enabled;
            slider.closest("div")?.classList.toggle("widget-control-disabled", !enabled);
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
        const x = Number(this.parameters.select_x) + index;
        const y = this.parameters.select_y;

        if (y === "") {
            this.send_control_change(this.parameters.parameter, value, x);
            return;
        }

        this.send_control_change(
            this.parameters.parameter,
            value,
            x,
            Math.trunc(Number(y))
        );
    }

    _syncAllSliders(value) {
        const sliders = this._getSliders();
        const count = Number(this.parameters.count);

        for (let i = 0; i < count; i += 1) {
            this._sendControlValue(value, i);
            if (sliders[i]) {
                sliders[i].value = value;
            }
        }

        if (this.parameters.show_values) {
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
        const shouldSync = Number(this.parameters.count) > 1 && (shiftPressed || this.sync);

        if (!shouldSync) {
            this._sendControlValue(value, index);
            if (this.parameters.show_values) {
                this._updateValueLabels();
            }
            return;
        }

        this._syncAllSliders(value);
    }

    updateAll() {
        super.updateAll();

        const container = this.firstChild;
        const count = Number(this.parameters.count);

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
        const step = Number(this.parameters.step);

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
            value.style.display = this.parameters.show_values ? "block" : "none";
        }

        this._updateValueLabels();
        this._syncEnabledState();

        this._bindKeyHandlersOnce();

        sliders.forEach((slider, index) => {
            slider.oninput = (event) => {
                this.slider_moved(slider.value, index, event.shiftKey);
            };

            const stopWidgetPropagation = (event) => {
                if (main.edit_mode) {
                    const component = this.parentElement;
                    const componentName = component?.dataset?.name || component?.id;

                    if (event.type === "mousedown") {
                        if (componentName) {
                            selector.selectItems([componentName], null, event.shiftKey);
                        }
                    }

                    if (event.detail === 2 && (event.type === "mousedown" || event.type === "click")) {
                        if (componentName) {
                            selector.selectItems([componentName], null, event.shiftKey);
                        }
                        inspector.toggleComponent();
                    }
                this.is_active = false;
                this.active_until = 0;
                event.stopPropagation();
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
        if (this.parameters.show_values) {
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

                const selectedY = Math.trunc(Number(this.parameters.select_y));
                let x = Number(this.parameters.select_x);

                for (const slider of sliders) {
                    slider.value = data[selectedY]?.[x] ?? slider.value;
                    x += 1;
                }
                return;
            }

            const values = isMatrix ? data[0] : (Array.isArray(data) ? data : [data]);
            let x = Number(this.parameters.select_x);
            for (const slider of sliders) {
                slider.value = values[x] ?? slider.value;
                x += 1;
            }

            if (this.parameters.show_values) {
                this._updateValueLabels();
            }
        }
        catch (err) {
        }
    }
}

webui_widgets.add("webui-widget-slider-horizontal", WebUIWidgetSliderHorizontal);
