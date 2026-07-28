class WebUIWidgetControl extends WebUIWidget
{
    normalizeControlIndex(value, fallback=0)
    {
        const number = Number(value);
        return Number.isFinite(number) ? Math.max(0, Math.trunc(number)) : fallback;
    }

    getSelectX(offset=0)
    {
        return this.normalizeControlIndex(this.parameters.select_x) + offset;
    }

    getSelectY(fallback="")
    {
        const value = this.parameters.select_y;
        if(value === undefined || value === null || value === "")
            return fallback;
        return this.normalizeControlIndex(value, fallback);
    }

    getSelectedSourceValue(sourceName, fallback=undefined, offset=0)
    {
        const source = this.getSource(sourceName, fallback);
        if(source === undefined || source === null)
            return fallback;

        const x = this.getSelectX(offset);
        const y = this.getSelectY();
        if(Array.isArray(source) && Array.isArray(source[0]))
            return source[y === "" ? 0 : y]?.[x] ?? fallback;
        if(Array.isArray(source))
            return source[x] ?? fallback;
        return offset === 0 ? source : fallback;
    }

    isControlEnabled(sourceName="enabled_source")
    {
        if(!this.parameters[sourceName])
            return true;
        const source = this.getSource(sourceName, 1);
        const value = Array.isArray(source) ? (Array.isArray(source[0]) ? source[0][0] : source[0]) : source;
        return Number(value) !== 0;
    }

    syncControlEnabledState(elements=[], containerSelector="")
    {
        const interactive = this.isControlEnabled() && !main.edit_mode;
        this.classList.toggle("widget-control-disabled", !interactive);
        for(const element of elements)
        {
            element.disabled = !interactive;
            element.style.pointerEvents = main.edit_mode ? "none" : "";
            if(containerSelector)
                element.closest(containerSelector)?.classList.toggle("widget-control-disabled", !interactive);
        }
        return interactive;
    }

    sendIndexedControlChange(parameter, value, offset=0)
    {
        if(!parameter)
            return;
        const x = this.getSelectX(offset);
        const y = this.getSelectY();
        if(y === "")
            this.send_control_change(parameter, value, x);
        else
            this.send_control_change(parameter, value, x, y);
    }

};
