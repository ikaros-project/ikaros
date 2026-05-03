class WebUIWidgetTable extends WebUIWidget {
    static template() {
        return [
            { 'name': "TABLE", 'control': 'header' },
            { 'name': 'title', 'default': "Default Title", 'type': 'string', 'control': 'textedit' },
            { 'name': 'source', 'default': "", 'type': 'source', 'control': 'textedit' },

            { 'name': "STYLE", 'control': 'header' },
            { 'name': 'label_x', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'label_y', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'direction', 'default': "normal", 'type': 'string', 'control': 'menu', 'options': "normal,flip x/y" },
            { 'name': 'decimals', 'default': 4, 'type': 'int', 'control': 'textedit' },
            { 'name': 'colorize', 'default': true, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'scrollable', 'default': false, 'type': 'bool', 'control': 'checkbox' }
        ]
    };

    static html() {
        return `<div class="table-widget-root"><div class="table-slice-controls"></div><table></table><div class="widget-overlay"><div class="widget-overlay-text"></div></div></div>`;
    }

    requestData(data_set) {
        this.addSource(data_set, this.parameters.source);
        this.addSourceMetadata(data_set, this.parameters.source);
    }

    reshapeTable(r, c, rLabel, cLabel) {
        this.xHeader = []
        this.yHeader = []
        this.tData = []
        for (var i = 0; i < r; i++)
            this.tData[i] = new Array(c);

        while (this.table.rows.length)
            this.table.deleteRow(-1);

        for (let j = 0; j < r; j++) {
            let new_row = this.table.insertRow(-1);
            for (let i = 0; i < c; i++) {
                let cell = new_row.insertCell(i);
                cell.innerHTML = "-";
                this.tData[j][i] = cell
            }
        }
        if (rLabel) {
            let new_row = this.table.insertRow(0);
            for (let i = 0; i < c; i++) {
                let cell = new_row.insertCell(i);
                cell.innerHTML = "-";
                this.xHeader.push(cell)
            }
        }
        if (cLabel) {
            for (let i = 0; i < this.table.rows.length; i++) {
                let cell = this.table.rows[i].insertCell(0);
                if (!(rLabel && i == 0)) {
                    cell.innerHTML = "-";
                    this.yHeader.push(cell)
                }
            }
        }
    }
    labelList(labels) {
        if (Array.isArray(labels))
            return labels.map((label) => String(label ?? "").trim());
        return String(labels ?? "").split(',').map((label) => label.trim()).filter((label) => label !== "");
    }
    hasLabels(labels) {
        return this.labelList(labels).some((label) => label !== "");
    }
    fillLabels(type, label_x, xCells, label_y, yCells) {
        if (this.hasLabels(label_x) || this.hasLabels(label_y)) {
            var xLabel = this.labelList(label_x).filter((label) => label !== "");
            var yLabel = this.labelList(label_y).filter((label) => label !== "");
            if (xLabel.length === 0)
                xLabel = [""];
            if (yLabel.length === 0)
                yLabel = [""];
            var ix

            this.table.style.border = "none"
            if (label_x && label_y)
                this.table.rows[0].cells[0].style.border = "none"

            if (type == "normal") {
                ix = 0
                for (var i = 0; i < xCells.length; i++) {
                    ix = (ix >= xLabel.length) ? 0 : ix;
                    xCells[i].innerHTML = xLabel[ix++]
                }
                ix = 0
                for (var i = 0; i < yCells.length; i++) {
                    ix = (ix >= yLabel.length) ? 0 : ix;
                    yCells[i].innerHTML = yLabel[ix++]
                    yCells[i].style.borderBottom = "none";
                }
            }
            else {
                ix = 0
                for (var i = 0; i < xCells.length; i++) {
                    ix = (ix >= yLabel.length) ? 0 : ix;
                    xCells[i].innerHTML = yLabel[ix++]

                }
                ix = 0
                for (var i = 0; i < yCells.length; i++) {
                    ix = (ix >= xLabel.length) ? 0 : ix;
                    yCells[i].innerHTML = xLabel[ix++]
                    yCells[i].style.borderBottom = "none";
                }
            }
        }
    }
    scrollable() {
        if (this.toBool(this.parameters.scrollable))
            this.div.style.overflow = "auto";
        else
            this.div.style.overflow = "inherit";
    }
    init() {
        this.div = this.querySelector('.table-widget-root');
        this.sliceControls = this.querySelector('.table-slice-controls');
        this.table = this.querySelector('table');
        this.loaded = false;
        this.overlay = false;
        this._tableShapeKey = "";
        this.sliceSelections = [];
        this.metadata = null;
    }
    getMatrixShape(data) {
        const shape = [];
        let current = data;
        while (Array.isArray(current)) {
            shape.push(current.length);
            if (current.length === 0)
                break;
            current = current[0];
        }
        return shape;
    }
    getDisplayState(data) {
        const shape = this.getMatrixShape(data);
        if (shape.length === 0)
            return null;

        const leadingShape = shape.slice(0, Math.max(0, shape.length - 2));
        this.sliceSelections.length = leadingShape.length;

        let matrix = data;
        for (let i = 0; i < leadingShape.length; i++) {
            if (!Array.isArray(matrix) || matrix.length === 0)
                return null;
            const maxIndex = matrix.length - 1;
            const priorIndex = Number(this.sliceSelections[i]);
            const nextIndex = Number.isFinite(priorIndex) ? Math.min(Math.max(priorIndex, 0), maxIndex) : 0;
            this.sliceSelections[i] = nextIndex;
            matrix = matrix[nextIndex];
        }

        if (shape.length === 1)
            matrix = [data];

        if (!Array.isArray(matrix) || matrix.length === 0)
            return null;
        if (!Array.isArray(matrix[0]))
            return null;

        let size_y = matrix.length;
        let size_x = matrix[0].length;
        if (!size_x)
            return null;

        return {
            shape,
            leadingShape,
            matrix,
            size_y,
            size_x
        };
    }
    getMetadataDimensionLabels(dimension) {
        const labels = this.metadata?.labels;
        if (!Array.isArray(labels) || !Array.isArray(labels[dimension]))
            return [];
        return labels[dimension].map((label) => String(label ?? "").trim());
    }
    getDisplayLabels(displayState) {
        if (!displayState)
            return { x: [], y: [] };

        const explicitX = this.labelList(this.parameters.label_x);
        const explicitY = this.labelList(this.parameters.label_y);
        const rank = displayState.shape.length;
        const rowDimension = rank === 1 ? null : rank - 2;
        const columnDimension = rank === 1 ? 0 : rank - 1;

        return {
            x: explicitX.length > 0 ? explicitX : this.getMetadataDimensionLabels(columnDimension),
            y: explicitY.length > 0 ? explicitY : (rowDimension === null ? [] : this.getMetadataDimensionLabels(rowDimension))
        };
    }
    getShapeKey(displayState) {
        if (!displayState)
            return "";

        const labels = this.getDisplayLabels(displayState);
        return `${this.parameters.direction}:${displayState.shape.join("x")}:${displayState.size_y}x${displayState.size_x}:${JSON.stringify(labels)}`;
    }
    renderSliceControls(displayState) {
        if (!this.sliceControls)
            return;

        this.sliceControls.replaceChildren();
        const leadingShape = displayState ? displayState.leadingShape : [];
        this.sliceControls.style.display = leadingShape.length > 0 ? "flex" : "none";
        if (leadingShape.length === 0)
            return;

        leadingShape.forEach((dimensionSize, dimensionIndex) => {
            const dimensionLabels = this.getMetadataDimensionLabels(dimensionIndex);
            const control = document.createElement("label");
            control.className = "table-slice-control";

            const caption = document.createElement("span");
            caption.textContent = `Dim ${dimensionIndex + 1}`;

            const select = document.createElement("select");
            for (let optionIndex = 0; optionIndex < dimensionSize; optionIndex++) {
                const option = document.createElement("option");
                option.value = String(optionIndex);
                const label = dimensionLabels[optionIndex] ?? "";
                option.textContent = label === "" ? String(optionIndex) : label;
                select.appendChild(option);
            }
            select.value = String(this.sliceSelections[dimensionIndex] ?? 0);
            select.onchange = () => {
                this.sliceSelections[dimensionIndex] = Number(select.value);
                this.loaded = false;
                this.updateAll();
                this.update();
            };
            select.onmousedown = (evt) => {
                evt.stopPropagation();
            };
            select.onclick = (evt) => {
                evt.stopPropagation();
            };

            control.append(caption, select);
            this.sliceControls.appendChild(control);
        });
    }
    updateAll() {
        this.updateFrame();
        const source = this.getSource('source');
        this.metadata = this.getSourceMetadata('source', null);
        const displayState = this.getDisplayState(source);
        this.renderSliceControls(displayState);
        if (!displayState) {
            this.loaded = false;
            this._tableShapeKey = "";
            this.scrollable();
            return;
        }

        this.data = displayState.matrix;
        this.loaded = true;
        this._tableShapeKey = this.getShapeKey(displayState);
        const labels = this.getDisplayLabels(displayState);

        if (this.parameters.direction == "normal")
            this.reshapeTable(displayState.size_y, displayState.size_x, this.hasLabels(labels.x), this.hasLabels(labels.y));
        else
            this.reshapeTable(displayState.size_x, displayState.size_y, this.hasLabels(labels.y), this.hasLabels(labels.x));
        this.fillLabels(this.parameters.direction, labels.x, this.xHeader, labels.y, this.yHeader)
        this.scrollable()
    }
    update() {

        if (!this.loaded)
            this.updateAll()

        else {
            const source = this.getSource('source');
            this.metadata = this.getSourceMetadata('source', null);
            const displayState = this.getDisplayState(source);
            if (!displayState) {
                this.loaded = false;
                this.renderSliceControls(null);
                return;
            }
            const nextShapeKey = this.getShapeKey(displayState);
            if(nextShapeKey !== this._tableShapeKey)
            {
                this.loaded = false;
                this.updateAll();
                return;
            }

            this.data = displayState.matrix;
            let size_y = displayState.size_y;
            let size_x = displayState.size_x;
            if (!size_x)
                return;


            for (let j = 0; j < size_y; j++)
                for (let i = 0; i < size_x; i++)
                    if (this.parameters.direction == "normal") {
                        try {
                            this.tData[j][i].innerHTML = this.data[j][i].toFixed(this.parameters.decimals);
                        }
                        catch (err) {
                            this.tData[j][i].innerHTML = "-";
                        }
                        if (this.parameters.colorize)
                            this.tData[j][i].style.color = this.getColor(i, this.data[j][i]);
                        else
                            this.tData[j][i].style.color = this.getColor(i);
                    }
                    else {
                        try {
                            this.tData[i][j].innerHTML = this.data[j][i].toFixed(this.parameters.decimals);
                        }
                        catch (err) {
                            this.tData[i][j].innerHTML = "-";
                        }
                        if (this.parameters.colorize)
                            this.tData[i][j].style.color = this.getColor(i, this.data[j][i]);
                        else
                            this.tData[i][j].style.color = this.getColor(i);
                    }
        }
    }
};

webui_widgets.add('webui-widget-table', WebUIWidgetTable);
