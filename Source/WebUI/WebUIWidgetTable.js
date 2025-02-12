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
            { 'name': 'scrollable', 'default': false, 'type': 'bool', 'control': 'checkbox' },

            { 'name': "FRAME", 'control': 'header' },
            { 'name': 'show_title', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'show_frame', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'style', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'frame-style', 'default': "", 'type': 'string', 'control': 'textedit' }
        ]
    };

    static html() {
        return `<div><table></table><div id=\"WidgetOverlay\"><div id=\"WidgetOverlayText\"></div> </div></div>`;
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
    fillLabels(type, label_x, xCells, label_y, yCells) {
        if (label_x || label_y) {
            var xLabel = label_x.split(',');
            var yLabel = label_y.split(',');
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
        this.div = this.querySelector('div');
        this.table = this.querySelector('table');
        this.loaded = false;
        this.overlay = false;
    }
    updateAll() {
        this.updateFrame();
        if (this.data = this.getSource('source')) {

            if(this.getMatrixRank(this.data) == 1) // TEMPORARY
                this.data = [this.data];

            this.loaded = true;
            let size_y = this.data.length;
            let size_x = this.data[0].length;

            if (this.parameters.direction == "normal")
                this.reshapeTable(size_y, size_x, this.parameters.label_x, this.parameters.label_y);
            else
                this.reshapeTable(size_x, size_y, this.parameters.label_y, this.parameters.label_x);
            this.fillLabels(this.parameters.direction, this.parameters.label_x, this.xHeader, this.parameters.label_y, this.yHeader)
        }
        this.scrollable()
    }
    update() {

        if (!this.loaded)
            this.updateAll()

        else {
            if (this.data = this.getSource('source')){

                if(this.getMatrixRank(this.data) == 1) // TEMPORARY
                    this.data = [this.data];
/*
                if(!Array.isArray(this.data))
                {
                    this.tData[0][0].innerHTML = "Not a 2D matrix";
                    return;
                }

                if(!Array.isArray(this.data[0]))
                {
                    this.tData[0][0].innerHTML = "Not a 2D matrix";
                    return;
                }

                if(typeof this.data[0][0] != 'number')
                {
                    this.tData[0][0].innerHTML = "Not a 2D matrix";
                    return;
                }
*/

                let size_y = this.data.length;
                let size_x = this.data[0].length;


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
    }
};

webui_widgets.add('webui-widget-table', WebUIWidgetTable);