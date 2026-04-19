class WebUIWidgetSwitch extends WebUIWidgetControl {
    static template() {
        return [
            { name: "SWITCH", control: "header" },
            { name: "title", default: "Switch Title", type: "string", control: "textedit" },
            { name: "labels", default: "Press", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter", default: "", type: "source", control: "textedit" },
            { name: "single_trig", default: true, type: "bool", control: "checkbox" },
            { name: "value", default: 1, type: "int", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },
            { name: "count", default: 1, type: "int", control: "textedit" },

            { name: "FRAME", control: "header" },
            { name: "show_title", default: false, type: "bool", control: "checkbox" },
            { name: "show_frame", default: false, type: "bool", control: "checkbox" },
            { name: "style", default: "", type: "string", control: "textedit" },
            { name: "frame-style", default: "", type: "string", control: "textedit" }
        ];
    }

    static html() {
        return '<div class="switch-list"></div>';
    }

    requestData(data_set) {
        data_set.add(this.parameters.parameter);
    }

    _getRows() {
        return this.querySelectorAll(".switch-row");
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
            Math.trunc(Number(y))
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
        const count = Math.max(1, Number(this.parameters.count) || 1);

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
    }

    update() {
        try {
            let data = this.getSource("parameter");
            if (!data) {
                return;
            }

            if (Array.isArray(data) && !Array.isArray(data[0])) {
                data = [data];
            }

            const rows = this._getRows();
            const selectedY = this._getSelectY();
            let x = this._getBaseSelectX();

            if (selectedY !== "") {
                const y = Math.trunc(Number(selectedY));
                if (!Array.isArray(data[y])) {
                    return;
                }

                rows.forEach((row) => {
                    const input = row.querySelector("input");
                    input.checked = ((data[y][x] ?? 0) > 0);
                    x += 1;
                });
                return;
            }

            rows.forEach((row) => {
                const input = row.querySelector("input");
                input.checked = ((data[x] ?? 0) > 0);
                x += 1;
            });
        }
        catch (err) {
        }
    }
}

webui_widgets.add("webui-widget-switch", WebUIWidgetSwitch);
