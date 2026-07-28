class WebUIWidgetSwitch extends WebUIWidgetControl {
    static template() {
        return [
            { name: "SWITCH", control: "header" },
            { name: "title", default: "Switch Title", type: "string", control: "textedit" },
            { name: "labels", default: "Press", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter", default: "", type: "source", control: "textedit" },
            { name: "enabled_source", default: "", type: "source", control: "textedit" },
            { name: "value", default: 1, type: "int", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },
            { name: "control_count", default: 1, type: "int", control: "textedit" }
        ];
    }

    static html() {
        return '<div class="switch-list"></div>';
    }

    requestData(data_set) {
        this.addSource(data_set, this.parameters.parameter);
        if (this.parameters.enabled_source) {
            this.addSource(data_set, this.parameters.enabled_source);
        }
    }

    _getRows() {
        return this.querySelectorAll(".switch-row");
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
        this.classList.toggle("widget-control-disabled", !enabled);
        for (const input of this.querySelectorAll("input")) {
            input.disabled = !enabled;
            input.closest(".switch-row")?.classList.toggle("widget-control-disabled", !enabled);
        }
    }

    _getBaseSelectX() {
        const value = Number(this.parameters.select_x);
        return Number.isFinite(value) ? Math.max(0, Math.trunc(value)) : 0;
    }

    _getSelectY() {
        if (this.parameters.select_y === undefined || this.parameters.select_y === null) {
            return "";
        }
        return this.parameters.select_y;
    }

    _sendControlValue(checked, index) {
        if (!this.parameters.parameter) {
            return;
        }

        const x = this._getBaseSelectX() + index;
        const y = this._getSelectY();
        const onValue = this.parameters.value;
        const offValue = 0;
        const value = checked ? onValue : offValue;

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

    _handleRowInput(rowIndex, event) {
        if (main.edit_mode) {
            const component = this.parentElement;
            const componentName = component?.dataset?.name || component?.id;
            if (componentName) {
                selector.selectItems([componentName], null, event.shiftKey);
            }
            if (event.detail === 2 || event.type === "dblclick") {
                inspector.toggleComponent();
            }
            event.preventDefault();
            event.stopPropagation();
            return;
        }

        this._sendControlValue(event.target.checked, rowIndex);
        event.stopPropagation();
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
            const row = document.createElement("label");
            row.className = "switch-row";

            const input = document.createElement("input");
            input.type = "checkbox";

            const text = document.createElement("span");
            text.className = "switch-label";

            row.append(input, text);
            container.appendChild(row);
        }

        const rawLabels = String(this.parameters.labels ?? "").trim();
        const labelParts = rawLabels === "" ? [] : rawLabels.split(",").map((item) => item.trim());
        const rows = this._getRows();

        rows.forEach((row, index) => {
            const input = row.querySelector("input");
            const text = row.querySelector(".switch-label");
            const fallbackLabel = count > 1 ? `${index}` : "";

            text.innerText = labelParts[index] ?? (labelParts[0] ?? fallbackLabel);

            input.oninput = (event) => {
                this._handleRowInput(index, event);
            };

            input.onmousedown = (event) => {
                if (main.edit_mode) {
                    const component = this.parentElement;
                    const componentName = component?.dataset?.name || component?.id;
                    if (componentName) {
                        selector.selectItems([componentName], null, event.shiftKey);
                    }
                    event.preventDefault();
                    event.stopPropagation();
                }
            };

            input.ondblclick = (event) => {
                if (main.edit_mode) {
                    const component = this.parentElement;
                    const componentName = component?.dataset?.name || component?.id;
                    if (componentName) {
                        selector.selectItems([componentName], null, event.shiftKey);
                    }
                    inspector.toggleComponent();
                    event.preventDefault();
                    event.stopPropagation();
                }
            };
        });

        this._syncEnabledState();
    }

    update() {
        this._syncEnabledState();
        const data = this.getSource("parameter");
        const rows = this._getRows();
        const selectedY = this._getSelectY();
        let values;
        if (selectedY !== "") {
            const configuredY = Number(selectedY);
            const y = Number.isFinite(configuredY) ? Math.max(0, Math.trunc(configuredY)) : 0;
            values = Array.isArray(data?.[y]) ? data[y] : [];
        }
        else if (Array.isArray(data) && Array.isArray(data[0]))
            values = data[0];
        else if (Array.isArray(data))
            values = data;
        else
            values = data === undefined || data === null ? [] : [data];

        let x = this._getBaseSelectX();
        rows.forEach((row) => {
            row.querySelector("input").checked = Number(values[x] ?? 0) > 0;
            x += 1;
        });
    }
}

webui_widgets.add("webui-widget-switch", WebUIWidgetSwitch);
