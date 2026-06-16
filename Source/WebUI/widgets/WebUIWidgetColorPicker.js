class WebUIWidgetColorPicker extends WebUIWidgetControl {
    static template() {
        return [
            { name: "COLOR PICKER", control: "header" },
            { name: "title", default: "Color", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter", default: "", type: "source", control: "textedit" },
            { name: "enableSource", default: "", type: "source", control: "textedit" },
            { name: "row_parameter", default: "", type: "source", control: "textedit" },

            { name: "STYLE", control: "header" },
            { name: "show_values", default: true, type: "bool", control: "checkbox" },
            { name: "step", default: 0.01, type: "float", control: "textedit" }
        ];
    }

    static html() {
        return `
            <div class="color-picker">
                <div class="color-picker-summary">
                    <input class="color-picker-native" type="color" value="#000000">
                    <span class="color-picker-hex">#000000</span>
                </div>
                <div class="color-picker-sliders"></div>
                <div class="color-picker-error"></div>
            </div>
        `;
    }

    requestData(data_set) {
        this.addSource(data_set, this.parameters.parameter);
        if (this.parameters.enableSource) {
            this.addSource(data_set, this.parameters.enableSource);
        }
        if (this.parameters.row_parameter) {
            this.addSource(data_set, this.parameters.row_parameter);
        }
    }

    _clamp01(value) {
        const number = Number(value);
        if (!Number.isFinite(number)) {
            return 0;
        }
        return Math.max(0, Math.min(1, number));
    }

    _toByte(value) {
        return Math.round(this._clamp01(value) * 255);
    }

    _componentToHex(value) {
        return this._toByte(value).toString(16).padStart(2, "0");
    }

    _rgbToHex(rgb) {
        return `#${this._componentToHex(rgb[0])}${this._componentToHex(rgb[1])}${this._componentToHex(rgb[2])}`;
    }

    _hexToRgb(hex) {
        const value = String(hex ?? "").replace("#", "");
        if (!/^[0-9a-fA-F]{6}$/.test(value)) {
            return [0, 0, 0];
        }

        return [
            parseInt(value.slice(0, 2), 16) / 255,
            parseInt(value.slice(2, 4), 16) / 255,
            parseInt(value.slice(4, 6), 16) / 255
        ];
    }

    _getRows() {
        return this.querySelectorAll(".color-picker-row");
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
        for (const input of this.querySelectorAll("input")) {
            input.disabled = !enabled;
        }
    }

    _setError(message) {
        const error = this.querySelector(".color-picker-error");
        if (!error) {
            return;
        }

        error.innerText = message;
        error.style.display = message ? "block" : "none";
    }

    _getRowParameterValue() {
        if (!this.parameters.row_parameter) {
            return "";
        }

        const rowSource = this.getSource("row_parameter", [[0]]);
        const rowValue = Array.isArray(rowSource)
            ? (Array.isArray(rowSource[0]) ? rowSource[0][0] : rowSource[0])
            : rowSource;
        const row = Math.trunc(Number(rowValue));

        return Number.isFinite(row) ? row : 0;
    }

    _readRgbFromData(data) {
        if (data === undefined || data === null) {
            return null;
        }

        let values = Array.isArray(data) ? data : [data];

        if (this.parameters.row_parameter) {
            if (!Array.isArray(data) || !Array.isArray(data[0])) {
                return null;
            }

            values = data[this._getRowParameterValue()];
        }
        else if (Array.isArray(data) && Array.isArray(data[0])) {
            values = data[0];
        }

        if (!Array.isArray(values) || values.length !== 3) {
            return null;
        }

        return [
            this._clamp01(values[0]),
            this._clamp01(values[1]),
            this._clamp01(values[2])
        ];
    }

    _sendChannelValue(channel, value) {
        if (!this.parameters.parameter) {
            return;
        }

        const row = this._getRowParameterValue();
        if (row === "") {
            this.send_control_change(this.parameters.parameter, this._clamp01(value), channel);
            return;
        }

        this.send_control_change(
            this.parameters.parameter,
            this._clamp01(value),
            channel,
            row
        );
    }

    _sendRgb(rgb) {
        rgb.forEach((value, channel) => {
            this._sendChannelValue(channel, value);
        });
    }

    _setDisplayedRgb(rgb) {
        const normalized = rgb.map((value) => this._clamp01(value));
        const hex = this._rgbToHex(normalized);
        const nativePicker = this.querySelector(".color-picker-native");
        const hexLabel = this.querySelector(".color-picker-hex");

        if (nativePicker) {
            nativePicker.value = hex;
        }
        if (hexLabel) {
            hexLabel.innerText = hex.toUpperCase();
        }

        this._getRows().forEach((row, channel) => {
            const slider = row.querySelector("input[type=range]");
            const value = row.querySelector(".color-picker-value");
            if (slider) {
                slider.value = normalized[channel];
            }
            if (value) {
                value.innerText = normalized[channel].toFixed(2);
                value.style.display = this.parameters.show_values ? "block" : "none";
            }
        });
    }

    _markActive() {
        this.is_active = true;
        this.active_until = Date.now() + 500;
    }

    _stopWidgetPropagation(event) {
        if (main.edit_mode) {
            const component = this.parentElement;
            const componentName = component?.dataset?.name || component?.id;

            if (event.type === "mousedown" && componentName) {
                selector.selectItems([componentName], null, event.shiftKey);
            }

            if (event.detail === 2 && (event.type === "mousedown" || event.type === "click")) {
                if (componentName) {
                    selector.selectItems([componentName], null, event.shiftKey);
                }
                inspector.toggleComponent();
            }

            this.is_active = false;
            this.active_until = 0;
            event.preventDefault();
            event.stopPropagation();
            return;
        }

        this.is_active = false;
        this.active_until = Date.now() + 500;
        event.stopPropagation();
    }

    updateAll() {
        super.updateAll();

        const sliderContainer = this.querySelector(".color-picker-sliders");
        const labels = ["R", "G", "B"];

        while (sliderContainer.childElementCount > labels.length) {
            sliderContainer.removeChild(sliderContainer.lastElementChild);
        }

        while (sliderContainer.childElementCount < labels.length) {
            const row = document.createElement("div");
            row.className = "color-picker-row";

            const label = document.createElement("span");
            label.className = "color-picker-label";

            const slider = document.createElement("input");
            slider.type = "range";
            slider.min = 0;
            slider.max = 1;

            const value = document.createElement("span");
            value.className = "color-picker-value";

            row.append(label, slider, value);
            sliderContainer.appendChild(row);
        }

        this._getRows().forEach((row, channel) => {
            const label = row.querySelector(".color-picker-label");
            const slider = row.querySelector("input[type=range]");

            label.innerText = labels[channel];
            slider.step = Number(this.parameters.step) || 0.01;
            slider.oninput = () => {
                this._markActive();
                const current = this._readRgbFromData(this.getSource("parameter")) ?? [0, 0, 0];
                current[channel] = this._clamp01(slider.value);
                this._setDisplayedRgb(current);
                this._sendChannelValue(channel, current[channel]);
            };

            slider.onmousedown = (event) => this._stopWidgetPropagation(event);
            slider.onmouseup = (event) => this._stopWidgetPropagation(event);
            slider.onclick = (event) => this._stopWidgetPropagation(event);
        });

        const nativePicker = this.querySelector(".color-picker-native");
        nativePicker.oninput = () => {
            this._markActive();
            const rgb = this._hexToRgb(nativePicker.value);
            this._setDisplayedRgb(rgb);
            this._sendRgb(rgb);
        };
        nativePicker.onchange = () => {
            this.is_active = false;
            this.active_until = Date.now() + 500;
        };
        nativePicker.onblur = () => {
            this.is_active = false;
        };
        nativePicker.onmousedown = (event) => this._stopWidgetPropagation(event);
        nativePicker.onmouseup = (event) => this._stopWidgetPropagation(event);
        nativePicker.onclick = (event) => this._stopWidgetPropagation(event);

        this._syncEnabledState();
    }

    update() {
        this._syncEnabledState();

        if (this.is_active || Date.now() < (this.active_until || 0)) {
            return;
        }

        const rgb = this._readRgbFromData(this.getSource("parameter"));
        if (!rgb) {
            this._setError("ColorPicker requires RGB width 3.");
            return;
        }

        this._setError("");
        this._setDisplayedRgb(rgb);
    }
}

webui_widgets.add("webui-widget-color-picker", WebUIWidgetColorPicker);
