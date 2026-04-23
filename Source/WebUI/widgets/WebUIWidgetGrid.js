class WebUIWidgetGrid extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "GRID", 'control':'header'},
            
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'red', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'green', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'blue', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'order', 'default':"row", 'type':'string', 'control': 'menu', 'options': "row,col"},
            {'name':'min', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':2, 'type':'float', 'control': 'textedit'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'label_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'labelWidth', 'default':100, 'type':'int', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},
            
            {'name':'command', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'interaction', 'default':"toggle", 'type':'string', 'control': 'menu', 'options': "toggle,slider"},
            {'name':'valueHigh', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'valueLow', 'default':0, 'type':'float', 'control': 'textedit'},
            
            {'name': "STYLE", 'control':'header'},

            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':"gray", 'type':'string', 'control': 'menu', 'options': "gray,fire,spectrum,custom,rgb"},
            {'name':'colorTable', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'shape', 'default':"rectangle", 'type':'string', 'control': 'menu', 'options': "rectangle,square,circle"},
            {'name':'size', 'default':1, 'type':'float', 'control': 'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},

            {'name':'scales', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no,invisible", 'class':'true'},
            {'name':'min_x', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max_x', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'min_y', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max_y', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'flipXAxis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flipYAxis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flipXCanvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flipYCanvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
        ]
    }

    init()
    {
        super.init();
        this.data = [];
        this.displayData = [];
        this.sliderInteraction = null;

        this.onclick = function (evt)
        {
            if(this.parameters.interaction === "slider")
                return;
            this.handleGridClick(evt);
        }

        this.onpointerdown = (evt) =>
        {
            if(this.parameters.interaction !== "slider")
                return;
            this.beginSliderInteraction(evt);
        };

        this._boundSliderMove = (evt) =>
        {
            this.updateSliderInteraction(evt);
        };

        this._boundSliderEnd = () =>
        {
            this.endSliderInteraction();
        };

        this._boundSliderClickSuppressor = (evt) =>
        {
            evt.preventDefault();
            evt.stopPropagation();
        };
    }

    disconnectedCallback()
    {
        if (typeof super.disconnectedCallback === "function")
            super.disconnectedCallback();
        this.endSliderInteraction();
    }

    hasDrawableGrid()
    {
        return Array.isArray(this.displayData) && Array.isArray(this.displayData[0]) && this.displayData.length > 0 && this.displayData[0].length > 0;
    }

    getGridMetrics()
    {
        if(!this.hasDrawableGrid())
            return null;

        const hasLabels = String(this.parameters.labels ?? "").trim() !== "";
        const labelWidth = hasLabels ? parseInt(this.parameters.labelWidth) : 0;
        const rect = this.canvasElement.getBoundingClientRect();
        const rows = this.displayData.length;
        const cols = this.displayData[0].length;
        const usableWidth = rect.width - this.format.spaceLeft - this.format.spaceRight - labelWidth;
        const usableHeight = rect.height - this.format.spaceTop - this.format.spaceBottom;
        if(usableWidth <= 0 || usableHeight <= 0 || rows <= 0 || cols <= 0)
            return null;

        return {
            rect,
            rows,
            cols,
            labelWidth,
            usableWidth,
            usableHeight,
            cellWidth: usableWidth / cols,
            cellHeight: usableHeight / rows
        };
    }

    getGridCellFromEvent(evt)
    {
        const metrics = this.getGridMetrics();
        if(!metrics)
            return null;

        const x = Math.floor(metrics.cols * (evt.clientX - metrics.rect.left - this.format.spaceLeft - metrics.labelWidth) / metrics.usableWidth);
        const y = Math.floor(metrics.rows * (evt.clientY - metrics.rect.top - this.format.spaceTop) / metrics.usableHeight);
        if(x < 0 || x >= metrics.cols || y < 0 || y >= metrics.rows)
            return null;

        return {x, y, metrics};
    }

    showGridValues()
    {
        let s = "";
        for(let r of this.data)
        {
            for(let c of r)
                s += c+"\t";
            s += "\n"
        }
        alert(s);
    }

    handleGridClick(evt)
    {
            if(main.edit_mode)
                return;
            if(!this.hasDrawableGrid())
                return;
            
            if(!this.parameters.command && !this.parameters.parameter)
            {
                this.showGridValues();
                return;
            }

            const cell = this.getGridCellFromEvent(evt);
            if(!cell)
                return;
            const {x, y} = cell;

            if(this.parameters.command)
                this.send_command(this.parameters.command, this.parameters.valueHigh, x, y)
                //this.get("/command/"+this.parameters.command+"/"+x+"/"+y+"/"+this.parameters.valueHigh);
            
            else if(this.parameters.parameter)
            {
                if(this.displayData[y][x] < this.parameters.valueHigh)
                    this.send_control_change(this.parameters.parameter, this.parameters.valueHigh, x, y);
                else
                    this.send_control_change(this.parameters.parameter, this.parameters.valueLow, x, y);
            }
        }

    getSliderStartValue(x, y)
    {
        const currentValue = Number(this.displayData?.[y]?.[x]);
        if(Number.isFinite(currentValue))
            return currentValue;
        const low = Number(this.parameters.valueLow);
        return Number.isFinite(low) ? low : 0;
    }

    beginSliderInteraction(evt)
    {
        if(main.edit_mode)
            return;
        if(this.parameters.command || !this.parameters.parameter)
            return;

        const cell = this.getGridCellFromEvent(evt);
        if(!cell)
            return;

        evt.preventDefault();
        evt.stopPropagation();

        const {x, y, metrics} = cell;
        this.sliderInteraction = {
            x,
            y,
            pointerId: evt.pointerId,
            startClientY: evt.clientY,
            startValue: this.getSliderStartValue(x, y),
            dragRange: Math.max(1, metrics.cellHeight * 2.0)
        };

        if(this.setPointerCapture && evt.pointerId !== undefined)
        {
            try {
                this.setPointerCapture(evt.pointerId);
            } catch (error) {
            }
        }

        window.addEventListener("pointermove", this._boundSliderMove, true);
        window.addEventListener("pointerup", this._boundSliderEnd, true);
        window.addEventListener("pointercancel", this._boundSliderEnd, true);
        window.addEventListener("click", this._boundSliderClickSuppressor, true);
        this.updateSliderInteraction(evt);
    }

    updateSliderInteraction(evt)
    {
        if(!this.sliderInteraction || !this.parameters.parameter)
            return;

        evt.preventDefault();
        evt.stopPropagation();

        const low = Number(this.parameters.valueLow);
        const high = Number(this.parameters.valueHigh);
        const rangeMin = Math.min(Number.isFinite(low) ? low : 0, Number.isFinite(high) ? high : 1);
        const rangeMax = Math.max(Number.isFinite(low) ? low : 0, Number.isFinite(high) ? high : 1);
        const span = rangeMax - rangeMin;
        const delta = (this.sliderInteraction.startClientY - evt.clientY) / this.sliderInteraction.dragRange;
        const nextValue = Math.max(rangeMin, Math.min(rangeMax, this.sliderInteraction.startValue + delta * span));

        if(Array.isArray(this.displayData?.[this.sliderInteraction.y]))
            this.displayData[this.sliderInteraction.y][this.sliderInteraction.x] = nextValue;

        this.send_control_change(this.parameters.parameter, nextValue, this.sliderInteraction.x, this.sliderInteraction.y);
        this.redrawGrid();
    }

    endSliderInteraction()
    {
        if(!this.sliderInteraction)
            return;

        const pointerId = this.sliderInteraction.pointerId;
        window.removeEventListener("pointermove", this._boundSliderMove, true);
        window.removeEventListener("pointerup", this._boundSliderEnd, true);
        window.removeEventListener("pointercancel", this._boundSliderEnd, true);
        setTimeout(() =>
        {
            window.removeEventListener("click", this._boundSliderClickSuppressor, true);
        }, 0);
        if(this.releasePointerCapture && pointerId !== undefined)
        {
            try {
                this.releasePointerCapture(pointerId);
            } catch (error) {
            }
        }
        this.sliderInteraction = null;
    }

    redrawGrid()
    {
        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.translate(this.format.marginLeft, this.format.marginTop);
        this.drawHorizontal(1, 1);
    }

    transposeMatrix(matrix)
    {
        if(!Array.isArray(matrix) || matrix.length === 0 || !Array.isArray(matrix[0]))
            return matrix;

        const rows = matrix.length;
        const cols = matrix[0].length;
        const transposed = Array.from({length: cols}, () => Array(rows));
        for(let y = 0; y < rows; y++)
            for(let x = 0; x < cols; x++)
                transposed[x][y] = matrix[y][x];
        return transposed;
    }

    getDisplayData(data)
    {
        if(this.parameters.order !== "col")
            return data;

        if(this.parameters.fill == "rgb")
        {
            if(!Array.isArray(data))
                return data;
            return data.map((channel) => this.transposeMatrix(channel));
        }

        return this.transposeMatrix(data);
    }

    channelToHex(value)
    {
        const numeric = Number(value);
        const byte = Math.max(0, Math.min(255, Math.round(255 * (Number.isFinite(numeric) ? numeric : 0))));
        return byte.toString(16).padStart(2, "0");
    }

    drawPlotHorizontal(width, height, index, transform)
    {
        let d = this.displayData;
        let rows = 0;
        let cols = 0;
        if (!Array.isArray(d) || d.length === 0)
            return;
        
        if(this.parameters.fill == "rgb")
        {
            if(!Array.isArray(d[0]) || !Array.isArray(d[0][0]))
                return;
            rows = d[0].length;
            cols = d[0][0].length;
        }
        else
        {
            if(!Array.isArray(d[0]))
                return;
            rows = d.length;
            cols = d[0].length;
        }
        
        this.canvas.lineWidth = this.format.lineWidth;
        this.canvas.textAlign = 'left';
        this.canvas.textBaseline = 'middle';

        let ct = LUT_gray;
        if(this.parameters.fill == 'fire')
            ct = LUT_fire;
        else if(this.parameters.fill == 'spectrum')
            ct = LUT_spectrum;

        if(String(this.parameters.colorTable ?? "").trim() != "")
        {
            ct = String(this.parameters.colorTable).split(',').map((entry) => entry.trim()).filter((entry) => entry !== "");
            if (ct.length === 0)
                ct = LUT_gray;
        }

        let labels = String(this.parameters.labels ?? "").trim() === "" ? [] : String(this.parameters.labels).split(',');
        let ln = labels.length;
        let ls = (ln ? parseInt(this.parameters.labelWidth) : 0);
        let n = ct.length;
        let dx = (width-ls)/cols;
        let dy = height/rows;
        let sx = dx*this.parameters.size;
        let sy = dy*this.parameters.size;

        if(this.parameters.shape == 'square' || this.parameters.shape == 'circle')
        {
            let minimum = Math.min(sx, sy);
            sx = minimum;
            sy = minimum;
        }

        if(this.parameters.fill == "rgb")
        {
            for(var i=0; i<rows; i++)
                {
                    if(ln)
                    {
                        this.canvas.fillStyle = "black";    // FIXME: Should really use the default color from the stylesheet
                        this.canvas.fillText((labels[i % ln] ?? "").trim(), 0, dy*i+dy/2);
                    }

                    for(var j=0; j<cols; j++)
                    {
                        this.setColor(i+j);
                        this.canvas.beginPath();
                        try {
                            let r = this.channelToHex(d[0][i][j]);
                            let g = this.channelToHex(d[1][i][j]);
                            let b = this.channelToHex(d[2][i][j]);
                            
                            this.canvas.fillStyle = '#'+r+g+b;
                        }  catch (error) {
                            this.canvas.fillStyle = "black";
                        }
                        if(this.parameters.shape == 'circle')
                            this.canvas.arc(ls+dx*j+dx/2, dy*i+dy/2, sx/2, 0, 2*Math.PI);
                        else
                            this.canvas.rect(ls+dx*j+dx/2-sx/2, dy*i+dy/2-sy/2, sx, sy);

                        this.canvas.fill();
                        this.canvas.stroke();

                        if(this.element_labels)
                        {
                            let lbl = this.element_labels[i*cols+j];
                            if(lbl)
                            {
                                this.canvas.fillStyle = "black";
                                this.canvas.textBaseline = "middle";
                                this.canvas.textAlign = "center";
                                this.canvas.font = '24px Arial';
                                this.canvas.fillText(lbl, ls+dx*j+dx/2, dy*i+dy/2, sx-20);
                            }
                        }
                    }
                }
        }
        else
        {
            for(var i=0; i<rows; i++)
            {
                if(ln)
                {
                    this.canvas.fillStyle = "black";    // FIXME: Should really use the default color form the stylesheet
                    this.canvas.fillText((labels[i % ln] ?? "").trim(), 0, dy*i+dy/2);
                }

                for(var j=0; j<cols; j++)
                {
                    this.setColor(i+j);
                    this.canvas.beginPath();
                    try {
                        let f = (d[i][j]-this.parameters.min)/(this.parameters.max-this.parameters.min);
                        let ix = Math.min(Math.floor(n*f), n-1);
                        this.canvas.fillStyle = String(ct[ix] ?? "black").trim();
                    } catch (error) {
                        this.canvas.fillStyle = "black";
                    }
                           
                    if(this.parameters.shape == 'circle')
                        this.canvas.arc(ls+dx*j+dx/2, dy*i+dy/2, sx/2, 0, 2*Math.PI);
                    else
                        this.canvas.rect(ls+dx*j+dx/2-sx/2, dy*i+dy/2-sy/2, sx, sy);

                    this.canvas.fill();
                    this.canvas.stroke();

                    if(this.element_labels)
                    {
                        let lbl = this.element_labels[i*cols+j];
                        if(lbl)
                        {
                            this.canvas.fillStyle = "black";
                            this.canvas.textBaseline = "middle";
                            this.canvas.textAlign = "center";
                            this.canvas.font = '24px Arial';
                            this.canvas.fillText(lbl, ls+dx*j+dx/2, dy*i+dy/2, sx-20);
                        }
                    }
                }
            }
        }
    }

    update()
    {
        if(this.parameters.fill == "rgb")
        {
            this.data = [this.getSource('red'), this.getSource('green'), this.getSource('blue')];
            if(!this.data[0] || !this.data[1] || !this.data[2])
                return;
            if(!Array.isArray(this.data[0]) || !Array.isArray(this.data[1]) || !Array.isArray(this.data[2]))
                return;
            if(this.data[0].length != this.data[1].length || this.data[1].length != this.data[2].length)
                return;
            this.displayData = this.getDisplayData(this.data);
            this.resetCanvasTransform(-0.5, -0.5);
            this.canvas.clearRect(0, 0, this.width, this.height);
            this.canvas.translate(this.format.marginLeft, this.format.marginTop); //
            this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
        }
        else if(this.data = this.getSource('source'))
        {
            this.displayData = this.getDisplayData(this.data);
            this.resetCanvasTransform(-0.5, -0.5);
            this.canvas.clearRect(0, 0, this.width, this.height);
            this.canvas.translate(this.format.marginLeft, this.format.marginTop); //
            this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
        }

        if(this.parameters.label_parameter)
        {
            let l = this.getSource('label_parameter');
            if(l)
            {
                if (Array.isArray(l))
                    this.element_labels = l.flat ? l.flat(Infinity).map((entry) => String(entry)) : l.map((entry) => String(entry));
                else
                    this.element_labels = String(l).split(',');
            }
        }
    }
};


webui_widgets.add('webui-widget-grid', WebUIWidgetGrid);
