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

    _syncEnabledState() {
        this.syncControlEnabledState(this.querySelectorAll("input"), ".switch-row");
    }

    _sendControlValue(checked, index) {
        if (!this.parameters.parameter) {
            return;
        }

        const onValue = this.parameters.value;
        const offValue = 0;
        const value = checked ? onValue : offValue;
        this.sendIndexedControlChange(this.parameters.parameter, value, index);
    }

    _handleRowInput(rowIndex, event) {
        if (main.edit_mode) {
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
                if (main.edit_mode)
                    return;
            };

            input.ondblclick = (event) => {
                if (main.edit_mode)
                    return;
            };
        });

        this._syncEnabledState();
    }

    update() {
        this._syncEnabledState();
        const rows = this._getRows();
        rows.forEach((row, index) => {
            row.querySelector("input").checked = Number(this.getSelectedSourceValue("parameter", 0, index)) > 0;
        });
    }
}

webui_widgets.add("webui-widget-switch", WebUIWidgetSwitch);
