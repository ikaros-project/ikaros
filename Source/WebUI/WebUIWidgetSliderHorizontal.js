class WebUIWidgetSliderHorizontal extends WebUIWidgetControl {
    static template() {
        return [
            { name: "SLIDER HORIZONTAL", control: "header" },
            { name: "title", default: "Sliders", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter", default: "", type: "source", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },
            { name: "count", default: 1, type: "int", control: "textedit" },

            { name: "STYLE", control: "header" },
            { name: "labels", default: "", type: "string", control: "textedit" },
            { name: "min", default: 0, type: "float", control: "textedit" },
            { name: "max", default: 1, type: "float", control: "textedit" },
            { name: "step", default: 0.01, type: "float", control: "textedit" },
            { name: "show_values", default: false, type: "bool", control: "checkbox" },

            { name: "FRAME", control: "header" },
            { name: "show_title", default: false, type: "bool", control: "checkbox" },
            { name: "show_frame", default: false, type: "bool", control: "checkbox" },
            { name: "style", default: "", type: "string", control: "textedit" },
            { name: "frame-style", default: "", type: "string", control: "textedit" }
        ];
    }

    static html() {
        return '<div class="hranger"></div>';
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

    slider_moved(value, index = 0) {
        this.is_active = true;
        this._sendControlValue(value, index);

        if (!this.sync) {
            return;
        }

        const sliders = this._getSliders();
        const count = Number(this.parameters.count);

        for (let i = 0; i < count; i += 1) {
            this._sendControlValue(value, i);
            if (sliders[i]) {
                sliders[i].value = value;
            }
        }
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

        const min = Number(this.parameters.min);
        const max = Number(this.parameters.max);
        const step = Number(this.parameters.step);

        for (const slider of sliders) {
            slider.min = min;
            slider.max = max;
            slider.step = step;
        }

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

        this._bindKeyHandlersOnce();

        sliders.forEach((slider, index) => {
            slider.oninput = () => {
                this.slider_moved(slider.value, index);
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
                    event.stopPropagation();
                    return;
                }
                this.is_active = false;
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

        if (this.is_active) {
            return;
        }

        try {
            let data = this.getSource("parameter");

            if (Array.isArray(data) && !Array.isArray(data[0])) {
                data = [data];
            }

            if (!data || !data.length) {
                return;
            }

            const sliders = this._getSliders();

            if (this.parameters.select_y !== "") {
                const selectedY = Math.trunc(Number(this.parameters.select_y));
                let x = Number(this.parameters.select_x);

                for (const slider of sliders) {
                    slider.value = data[selectedY]?.[x] ?? slider.value;
                    x += 1;
                }
                return;
            }

            let x = Number(this.parameters.select_x);
            for (const slider of sliders) {
                slider.value = data[x] ?? slider.value;
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
