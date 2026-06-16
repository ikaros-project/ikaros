class WebUIWidgetSequenceGrid extends WebUIWidget
{
    static template()
    {
        return [
            {'name': "SEQUENCE GRID", 'control':'header'},
            {'name':'title', 'default':"Sequence Grid", 'type':'string', 'control': 'textedit'},

            {'name': "SOURCES", 'control':'header'},
            {'name':'sequence_names', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'playing', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'layout_width', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'color', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'columns', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'cell_gap', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'playing_color', 'default':"#67c1ff", 'type':'string', 'control': 'textedit'}
        ]
    }

    static html()
    {
        return `
            <style>
                .sequence-grid {
                    box-sizing: border-box;
                    width: 100%;
                    height: 100%;
                    display: grid;
                    align-content: stretch;
                    justify-content: stretch;
                    padding: 4px;
                    background: #f4f4f4;
                    color: #222;
                    user-select: none;
                    overflow: hidden;
                }

                .sequence-grid-cell {
                    min-width: 0;
                    min-height: 0;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    padding: 6px;
                    border: 1px solid #777;
                    border-radius: 4px;
                    background: #e8e8e8;
                    color: inherit;
                    font: 13px system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
                    line-height: 1.15;
                    text-align: center;
                    overflow: hidden;
                    text-overflow: ellipsis;
                    cursor: pointer;
                    transition: background-color 80ms linear, box-shadow 80ms linear, transform 80ms linear;
                }

                .sequence-grid-cell:hover {
                    filter: brightness(1.06);
                }

                .sequence-grid-cell.playing {
                    border-color: var(--sequence-grid-playing, #67c1ff);
                    box-shadow: inset 0 0 0 3px var(--sequence-grid-playing, #67c1ff), 0 0 0 1px rgba(0,0,0,0.22);
                    filter: brightness(1.12);
                    font-weight: 600;
                }

                .sequence-grid-cell:focus-visible {
                    outline: 2px solid #111;
                    outline-offset: -3px;
                }
            </style>
            <div class="sequence-grid"></div>
        `;
    }

    init()
    {
        super.init();
        this.render_signature = undefined;
        this.gridElement = this.querySelector(".sequence-grid");
    }

    disconnectedCallback()
    {
        if (typeof super.disconnectedCallback === "function")
            super.disconnectedCallback();
    }

    requestData(data_set)
    {
        this.addSource(data_set, this.parameters.sequence_names);
        this.addSource(data_set, this.parameters.playing);
        this.addSource(data_set, this.parameters.layout_width);
        this.addSource(data_set, this.parameters.color);
    }

    asFlatArray(value)
    {
        if(value == undefined)
            return [];
        if(Array.isArray(value))
            return value.flat ? value.flat(Infinity) : value.reduce((a, b) => a.concat(Array.isArray(b) ? this.asFlatArray(b) : b), []);
        return [value];
    }

    getSequenceNames()
    {
        const names = this.getSource("sequence_names", "");
        if(Array.isArray(names))
            return this.asFlatArray(names).map((name) => String(name));
        return String(names)
            .split(",")
            .map((name) => name.trim())
            .filter((name) => name !== "");
    }

    getPlaying()
    {
        return this.asFlatArray(this.getSource("playing", []))
            .map((value) => Number(value) > 0);
    }

    colorComponentToByte(value)
    {
        const number = Number(value);
        if(!Number.isFinite(number))
            return 0;
        if(number <= 1)
            return Math.round(Math.max(0, Math.min(1, number)) * 255);
        return Math.round(Math.max(0, Math.min(255, number)));
    }

    rgbToCss(row)
    {
        if(!Array.isArray(row) || row.length < 3)
            return "";
        const r = this.colorComponentToByte(row[0]);
        const g = this.colorComponentToByte(row[1]);
        const b = this.colorComponentToByte(row[2]);
        return `rgb(${r}, ${g}, ${b})`;
    }

    rgbTextColor(row)
    {
        if(!Array.isArray(row) || row.length < 3)
            return "";
        const r = this.colorComponentToByte(row[0]) / 255;
        const g = this.colorComponentToByte(row[1]) / 255;
        const b = this.colorComponentToByte(row[2]) / 255;
        const luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;
        return luminance > 0.55 ? "#111" : "#fff";
    }

    getSequenceColors()
    {
        const value = this.getSource("color", []);
        if(!Array.isArray(value))
            return [];
        if(value.length == 0)
            return [];
        if(Array.isArray(value[0]))
            return value;
        return [value];
    }

    getLayoutWidth(sequence_count)
    {
        let width = Number(this.parameters.columns);
        if(!Number.isFinite(width) || width <= 0)
            width = Number(this.asFlatArray(this.getSource("layout_width", []))[0]);
        if(!Number.isFinite(width) || width <= 0)
            width = 8;
        return Math.max(1, Math.min(Math.trunc(width), Math.max(1, sequence_count)));
    }

    triggerSequence(index)
    {
        if(main.edit_mode || !this.parameters.command)
            return;

        const columns = this.getLayoutWidth(Math.max(1, this.getSequenceNames().length));
        const x = index % columns;
        const y = Math.floor(index / columns);

        this.send_command(this.parameters.command, index, x, y);
        controller.flushCommandQueue();
        this.update();
    }

    createCell(index, label, playing, color)
    {
        const cell = document.createElement("button");
        cell.type = "button";
        cell.className = "sequence-grid-cell";
        if(playing)
            cell.classList.add("playing");
        this.applyCellColor(cell, color);
        cell.textContent = label;
        cell.title = label;
        cell.addEventListener("pointerdown", (event) =>
        {
            if(event.button !== undefined && event.button != 0)
                return;
            event.preventDefault();
            event.stopPropagation();
            this.triggerSequence(index);
        });
        cell.addEventListener("click", (event) =>
        {
            event.preventDefault();
            event.stopPropagation();
        });
        cell.addEventListener("keydown", (event) =>
        {
            if(event.key !== "Enter" && event.key !== " ")
                return;
            event.preventDefault();
            event.stopPropagation();
            this.triggerSequence(index);
        });
        return cell;
    }

    applyCellColor(cell, color)
    {
        const background = this.rgbToCss(color);
        cell.style.background = background || "";
        cell.style.color = background ? this.rgbTextColor(color) : "";
    }

    updateCellStates(playing, colors)
    {
        const cells = this.gridElement.querySelectorAll(".sequence-grid-cell");
        for(let i=0; i<cells.length; i++)
        {
            cells[i].classList.toggle("playing", playing[i] === true);
            this.applyCellColor(cells[i], colors[i]);
        }
    }

    update()
    {
        if(this.gridElement == undefined)
            this.gridElement = this.querySelector(".sequence-grid");
        if(this.gridElement == undefined)
            return;

        const names = this.getSequenceNames();
        const playing = this.getPlaying();
        const colors = this.getSequenceColors();
        const sequence_count = Math.max(names.length, playing.length) || colors.length;
        const columns = this.getLayoutWidth(sequence_count);
        const rows = Math.max(1, Math.ceil(sequence_count / columns));

        this.gridElement.style.gridTemplateColumns = `repeat(${columns}, minmax(0, 1fr))`;
        this.gridElement.style.gridTemplateRows = `repeat(${rows}, minmax(0, 1fr))`;
        this.gridElement.style.gap = `${Math.max(0, Number(this.parameters.cell_gap) || 0)}px`;
        this.gridElement.style.setProperty("--sequence-grid-playing", this.parameters.playing_color || "#67c1ff");

        const signature = JSON.stringify({names, sequence_count, columns, rows});
        if(signature != this.render_signature)
        {
            this.gridElement.replaceChildren();
            for(let i=0; i<sequence_count; i++)
                this.gridElement.appendChild(this.createCell(i, names[i] || `Sequence ${i+1}`, playing[i], colors[i]));
            this.render_signature = signature;
        }
        else
            this.updateCellStates(playing, colors);
    }
};

webui_widgets.add('webui-widget-sequence-grid', WebUIWidgetSequenceGrid);
