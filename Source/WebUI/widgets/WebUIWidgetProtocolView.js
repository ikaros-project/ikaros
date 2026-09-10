class WebUIWidgetProtocolView extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name':'PROTOCOL HISTORY', 'control':'header'},
            {'name':'title', 'default':'Protocol history', 'type':'string', 'control':'textedit'},
            {'name':'time_source', 'default':'', 'type':'source', 'control':'textedit'},
            {'name':'signal_source', 'default':'', 'type':'source', 'control':'textedit'},
            {'name':'count_source', 'default':'', 'type':'source', 'control':'textedit'},
            {'name':'trial_index_source', 'default':'', 'type':'source', 'control':'textedit'},
            {'name':'window_source', 'default':'', 'type':'source', 'control':'textedit'},
            {'name':'labels', 'default':'Response', 'type':'string', 'control':'textedit'},
            {'name':'colors', 'default':'#26758a,#d87928,#76539b,#388e5b', 'type':'string', 'control':'textedit'},
            {'name':'y_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'auto_range', 'default':'yes', 'type':'bool', 'control':'checkbox'}
        ];
    }

    init()
    {
        super.init();
        this.font = '12px Didact Gothic, sans-serif';
    }

    rows(value)
    {
        if(!Array.isArray(value))
            return [];
        if(value.length === 0)
            return [];
        return Array.isArray(value[0]) ? value : value.map((entry) => [entry]);
    }

    scalar(value, fallback=0)
    {
        while(Array.isArray(value))
            value = value[0];
        const number = Number(value);
        return Number.isFinite(number) ? number : fallback;
    }

    update()
    {
        const times = this.rows(this.getSource('time_source', []));
        const signals = this.rows(this.getSource('signal_source', []));
        const trials = this.rows(this.getSource('trial_index_source', []));
        const windows = this.rows(this.getSource('window_source', []));
        const count = Math.max(0, Math.min(signals.length,
            Math.trunc(this.scalar(this.getSource('count_source', 0), 0))));
        this.resetCanvasTransform();
        this.drawHistory(times, signals, trials, windows, count);
    }

    drawHistory(times, signals, trials, windows, count)
    {
        this.clearCanvas(0, 0);
        this.canvas.fillStyle = '#faf9f5';
        this.canvas.fillRect(0, 0, this.width, this.height);

        const left = 48;
        const right = 14;
        const top = 24;
        const bottom = 34;
        const plotWidth = Math.max(1, this.width - left - right);
        const plotHeight = Math.max(1, this.height - top - bottom);
        this.canvas.font = this.font;

        if(count < 1)
        {
            this.canvas.fillStyle = '#596169';
            this.canvas.fillText('Waiting for samples', left, top + 20);
            return;
        }

        const startTime = this.scalar(times[0], 0);
        const endTime = this.scalar(times[count - 1], startTime + 1);
        const duration = Math.max(1e-9, endTime - startTime);
        const x = (index) => left + plotWidth * (this.scalar(times[index], startTime) - startTime) / duration;

        let segmentStart = 0;
        for(let index = 1; index <= count; ++index)
        {
            const changed = index === count || this.scalar(trials[index], -1) !== this.scalar(trials[segmentStart], -1);
            if(!changed)
                continue;
            if(Math.trunc(this.scalar(trials[segmentStart], -1)) >= 0)
            {
                this.canvas.fillStyle = Math.trunc(this.scalar(trials[segmentStart], 0)) % 2 === 0 ? '#e8f0f1' : '#f1ece4';
                this.canvas.fillRect(x(segmentStart), top, Math.max(1, x(Math.min(index, count - 1)) - x(segmentStart)), plotHeight);
            }
            segmentStart = index;
        }

        for(let window = 0; window < (windows[0]?.length || 0); ++window)
        {
            this.canvas.fillStyle = window % 2 === 0 ? 'rgba(55,128,148,0.10)' : 'rgba(210,132,54,0.10)';
            for(let index = 0; index < count; ++index)
                if(Number(windows[index]?.[window]) > 0.5)
                    this.canvas.fillRect(x(index), top, Math.max(1, plotWidth / Math.max(1, count - 1)), plotHeight);
        }

        let minimum = Number(this.parameters.y_min);
        let maximum = Number(this.parameters.y_max);
        if(Boolean(this.parameters.auto_range))
        {
            minimum = Infinity;
            maximum = -Infinity;
            for(let index = 0; index < count; ++index)
                for(const value of signals[index] || [])
                    if(Number.isFinite(Number(value)))
                    {
                        minimum = Math.min(minimum, Number(value));
                        maximum = Math.max(maximum, Number(value));
                    }
            if(!Number.isFinite(minimum) || !Number.isFinite(maximum))
                [minimum, maximum] = [0, 1];
            if(minimum === maximum)
                maximum = minimum + 1;
        }
        const y = (value) => top + plotHeight * (1 - (Number(value) - minimum) / (maximum - minimum));

        this.canvas.strokeStyle = '#788189';
        this.canvas.lineWidth = 1;
        this.canvas.strokeRect(left, top, plotWidth, plotHeight);
        this.canvas.fillStyle = '#4f575e';
        this.canvas.fillText(maximum.toFixed(2), 4, top + 4);
        this.canvas.fillText(minimum.toFixed(2), 4, top + plotHeight);
        this.canvas.fillText(`${startTime.toFixed(1)} s`, left, top + plotHeight + 20);
        const endLabel = `${endTime.toFixed(1)} s`;
        this.canvas.fillText(endLabel, left + plotWidth - this.canvas.measureText(endLabel).width, top + plotHeight + 20);

        const colors = String(this.parameters.colors || '').split(',').map((color) => color.trim());
        const labels = String(this.parameters.labels || '').split(',').map((label) => label.trim());
        const channelCount = signals[0]?.length || 0;
        for(let channel = 0; channel < channelCount; ++channel)
        {
            this.canvas.beginPath();
            for(let index = 0; index < count; ++index)
            {
                const px = x(index);
                const py = y(signals[index]?.[channel]);
                if(index === 0)
                    this.canvas.moveTo(px, py);
                else
                    this.canvas.lineTo(px, py);
            }
            this.canvas.strokeStyle = colors[channel % Math.max(1, colors.length)] || '#26758a';
            this.canvas.lineWidth = 2;
            this.canvas.stroke();
            this.canvas.fillStyle = this.canvas.strokeStyle;
            this.canvas.fillText(labels[channel] || `Signal ${channel + 1}`, left + 8 + channel * 110, top + 14);
        }
    }
}

webui_widgets.add('webui-widget-protocolview', WebUIWidgetProtocolView);
