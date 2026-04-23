class WebUIWidgetSliderVertical extends WebUIWidgetControl {
    static template() {
        return [
            { name: "SLIDER VERTICAL", control: "header" },
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
            { name: "show_values", default: false, type: "bool", control: "checkbox" }
        ];
    }

    static html() {
        return '<div class="vranger"></div>';
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

    _layoutSliders() {
        const columns = this.firstChild?.children;
        if (!columns || columns.length === 0) {
            return;
        }

        for (const column of columns) {
            const label = column.querySelector(".slider_label");
            const slot = column.querySelector(".slider_slot");
            const slider = column.querySelector("input");
            const value = column.querySelector(".slider_value");

            if (!slider || !slot) {
                continue;
            }

            const labelHeight = label && getComputedStyle(label).display !== "none"
                ? label.getBoundingClientRect().height
                : 0;
            const valueHeight = value && getComputedStyle(value).display !== "none"
                ? value.getBoundingClientRect().height
                : 0;

            const slotStyle = getComputedStyle(slot);
            const slotPaddingTop = parseFloat(slotStyle.paddingTop) || 0;
            const slotPaddingBottom = parseFloat(slotStyle.paddingBottom) || 0;
            const slotHeight = slot.getBoundingClientRect().height;
            const fallbackHeight = column.getBoundingClientRect().height - labelHeight - valueHeight;
            const availableHeight = Math.max(
                24,
                (slotHeight || fallbackHeight) - slotPaddingTop - slotPaddingBottom
            );

            slider.style.width = `${availableHeight}px`;
        }
    }

    _updateValueLabels() {
        const columns = this.firstChild?.children ?? [];
        for (const column of columns) {
            const value = column.querySelector(".slider_value");
            const slider = column.querySelector("input");
            if (value && slider) {
                value.innerText = slider.value;
            }
        }
    }

    _getBaseSelectX() {
        return Number(this.parameters.select_x ?? 0);
    }

    _getSelectY() {
        if (this.parameters.select_y === undefined || this.parameters.select_y === null) {
            return "";
        }
        return this.parameters.select_y;
    }

    _sendControlValue(value, index) {
        const x = this._getBaseSelectX() + index;
        const y = this._getSelectY();

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

    slider_moved(value, index = 0, shiftPressed = false) {
        this.is_active = true;
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
            const column = document.createElement("div");

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
            column.append(label, slot, value);
            container.appendChild(column);
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

        this._layoutSliders();
        this._updateValueLabels();

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
            const selectedY = this._getSelectY();

            if (selectedY !== "") {
                const y = Math.trunc(Number(selectedY));
                let x = this._getBaseSelectX();

                for (const slider of sliders) {
                    slider.value = data[y]?.[x] ?? slider.value;
                    x += 1;
                }
                if (this.parameters.show_values) {
                    this._updateValueLabels();
                }
                return;
            }

            let x = this._getBaseSelectX();
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

webui_widgets.add("webui-widget-slider-vertical", WebUIWidgetSliderVertical);
