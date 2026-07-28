class WebUIWidgetGrid extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "GRID", 'control':'header'},
            
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'red_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'green_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'blue_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'order', 'default':"row", 'type':'string', 'control': 'menu', 'options': "row,col"},
            {'name':'value_min', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'value_max', 'default':2, 'type':'float', 'control': 'textedit'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'label_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'label_width', 'default':100, 'type':'int', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},
            
            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'interaction', 'default':"toggle", 'type':'string', 'control': 'menu', 'options': "toggle,slider"},
            {'name':'on_value', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'off_value', 'default':0, 'type':'float', 'control': 'textedit'},
            
            {'name': "STYLE", 'control':'header'},

            {'name':'stroke_color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'color_map', 'default':"gray", 'type':'string', 'control': 'menu', 'options': "gray,fire,spectrum,custom,rgb"},
            {'name':'color_map_colors', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'line_width', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'shape', 'default':"rectangle", 'type':'string', 'control': 'menu', 'options': "rectangle,square,circle"},
            {'name':'size', 'default':1, 'type':'float', 'control': 'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},

            {'name':'scale_visibility', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no,invisible", 'class':'true'},
            {'name':'x_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'x_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'y_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'flip_x_axis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_y_axis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_x_canvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_y_canvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
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
        this.endSliderInteraction();
        super.disconnectedCallback();
    }

    requestData(data_set)
    {
        super.requestData(data_set);
        this.addSourceMetadata(data_set, this.parameters.source);
        this.addSourceMetadata(data_set, this.parameters.red_source);
        this.addSourceMetadata(data_set, this.parameters.green_source);
        this.addSourceMetadata(data_set, this.parameters.blue_source);
    }

    hasDrawableGrid()
    {
        return Array.isArray(this.displayData) && Array.isArray(this.displayData[0]) && this.displayData.length > 0 && this.displayData[0].length > 0;
    }

    getGridMetrics()
    {
        if(!this.hasDrawableGrid())
            return null;

        const hasLabels = this.getDisplayLabels().length > 0;
        const configuredLabelWidth = Number(this.parameters.label_width);
        const label_width = hasLabels && Number.isFinite(configuredLabelWidth) ? Math.max(0, configuredLabelWidth) : 0;
        const rect = this.canvasElement.getBoundingClientRect();
        const rgb = this.parameters.color_map === "rgb";
        const rows = rgb ? this.displayData[0].length : this.displayData.length;
        const cols = rgb ? this.displayData[0][0]?.length : this.displayData[0].length;
        const usableWidth = rect.width - this.format.space_left - this.format.space_right - label_width;
        const usableHeight = rect.height - this.format.space_top - this.format.space_bottom;
        if(usableWidth <= 0 || usableHeight <= 0 || rows <= 0 || cols <= 0)
            return null;

        return {
            rect,
            rows,
            cols,
            label_width,
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

        const x = Math.floor(metrics.cols * (evt.clientX - metrics.rect.left - this.format.space_left - metrics.label_width) / metrics.usableWidth);
        const y = Math.floor(metrics.rows * (evt.clientY - metrics.rect.top - this.format.space_top) / metrics.usableHeight);
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
                this.send_command(this.parameters.command, this.parameters.on_value, x, y)
                //this.get("/command/"+this.parameters.command+"/"+x+"/"+y+"/"+this.parameters.on_value);
            
            else if(this.parameters.parameter)
            {
                if(this.displayData[y][x] < this.parameters.on_value)
                    this.send_control_change(this.parameters.parameter, this.parameters.on_value, x, y);
                else
                    this.send_control_change(this.parameters.parameter, this.parameters.off_value, x, y);
            }
        }

    getSliderStartValue(x, y)
    {
        const currentValue = Number(this.displayData?.[y]?.[x]);
        if(Number.isFinite(currentValue))
            return currentValue;
        const low = Number(this.parameters.off_value);
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

        this._sliderListenerRemovers = [
            this.addManagedListener(window, "pointermove", this._boundSliderMove, true),
            this.addManagedListener(window, "pointerup", this._boundSliderEnd, true),
            this.addManagedListener(window, "pointercancel", this._boundSliderEnd, true),
            this.addManagedListener(window, "click", this._boundSliderClickSuppressor, true)
        ];
        this.updateSliderInteraction(evt);
    }

    updateSliderInteraction(evt)
    {
        if(!this.sliderInteraction || !this.parameters.parameter)
            return;

        evt.preventDefault();
        evt.stopPropagation();

        const low = Number(this.parameters.off_value);
        const high = Number(this.parameters.on_value);
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
        const removers = this._sliderListenerRemovers || [];
        removers.slice(0, 3).forEach((remove) => remove());
        if(removers[3])
            setTimeout(removers[3], 0);
        this._sliderListenerRemovers = [];
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
        this.beginCanvasDraw();
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

    normalizeGridData(data)
    {
        if(!Array.isArray(data))
            return data;
        if(data.length === 0)
            return data;
        if(!Array.isArray(data[0]))
            return [data];
        return data;
    }

    getDisplayData(data)
    {
        data = this.normalizeGridData(data);

        if(this.parameters.order !== "col")
            return data;

        if(this.parameters.color_map == "rgb")
        {
            if(!Array.isArray(data))
                return data;
            return data.map((channel) => this.transposeMatrix(channel));
        }

        return this.transposeMatrix(data);
    }

    getMetadataLabels()
    {
        const labels = this.metadata?.labels;
        if(!Array.isArray(labels) || !Array.isArray(labels[0]))
            return [];
        return labels[0].map((label) => String(label ?? "").trim()).filter((label) => label !== "");
    }

    getDisplayLabels()
    {
        const explicitLabels = String(this.parameters.labels ?? "").trim();
        if(explicitLabels !== "")
            return explicitLabels.split(',').map((label) => label.trim()).filter((label) => label !== "");
        return this.getMetadataLabels();
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
        
        if(this.parameters.color_map == "rgb")
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
        
        this.canvas.lineWidth = this.format.line_width;
        this.canvas.textAlign = 'left';
        this.canvas.textBaseline = 'middle';

        let ct = LUT_gray;
        if(this.parameters.color_map == 'fire')
            ct = LUT_fire;
        else if(this.parameters.color_map == 'spectrum')
            ct = LUT_spectrum;

        if(String(this.parameters.color_map_colors ?? "").trim() != "")
        {
            ct = String(this.parameters.color_map_colors).split(',').map((entry) => entry.trim()).filter((entry) => entry !== "");
            if (ct.length === 0)
                ct = LUT_gray;
        }

        let labels = this.getDisplayLabels();
        let ln = labels.length;
        const configuredLabelWidth = Number(this.parameters.label_width);
        let ls = ln && Number.isFinite(configuredLabelWidth) ? Math.max(0, configuredLabelWidth) : 0;
        let n = ct.length;
        let dx = (width-ls)/cols;
        let dy = height/rows;
        const configuredSize = Number(this.parameters.size);
        const size = Number.isFinite(configuredSize) ? Math.max(0, configuredSize) : 1;
        let sx = dx*size;
        let sy = dy*size;

        if(this.parameters.shape == 'square' || this.parameters.shape == 'circle')
        {
            let minimum = Math.min(sx, sy);
            sx = minimum;
            sy = minimum;
        }

        if(this.parameters.color_map == "rgb")
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
                        const value = Number(d[i][j]);
                        const minimum = Number(this.parameters.value_min);
                        const maximum = Number(this.parameters.value_max);
                        const span = maximum - minimum;
                        const f = Number.isFinite(value) && Number.isFinite(span) && span !== 0 ? (value - minimum) / span : 0;
                        let ix = Math.max(0, Math.min(Math.floor(n*f), n-1));
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
        if(this.parameters.label_source)
        {
            const labels = this.getSource('label_source');
            if(Array.isArray(labels))
                this.element_labels = labels.flat ? labels.flat(Infinity).map((entry) => String(entry)) : labels.map((entry) => String(entry));
            else if(labels !== undefined && labels !== null)
                this.element_labels = String(labels).split(',');
            else
                this.element_labels = [];
        }
        else
            this.element_labels = [];

        this.clearCanvas();

        if(this.parameters.color_map == "rgb")
        {
            this.data = [this.getSource('red_source'), this.getSource('green_source'), this.getSource('blue_source')];
            if(!this.data[0] || !this.data[1] || !this.data[2])
            {
                this.displayData = [];
                return;
            }
            if(!Array.isArray(this.data[0]) || !Array.isArray(this.data[1]) || !Array.isArray(this.data[2]))
            {
                this.displayData = [];
                return;
            }
            if(this.data[0].length != this.data[1].length || this.data[1].length != this.data[2].length)
            {
                this.displayData = [];
                return;
            }
            this.metadata = this.getSourceMetadata('red_source', null);
            this.displayData = this.getDisplayData(this.data);
            this.canvas.translate(this.format.margin_left, this.format.margin_top); //
            this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
        }
        else
        {
            this.data = this.getSource('source');
            if(!Array.isArray(this.data) || this.data.length === 0)
            {
                this.displayData = [];
                return;
            }
            this.metadata = this.getSourceMetadata('source', null);
            this.displayData = this.getDisplayData(this.data);
            this.canvas.translate(this.format.margin_left, this.format.margin_top); //
            this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
        }
    }
};


webui_widgets.add('webui-widget-grid', WebUIWidgetGrid);
