class WebUIWidgetHeatMap extends WebUIWidgetGrid
{
    static template()
    {
        const template = WebUIWidgetGrid.template().map((entry) => ({...entry}));
        template[0].name = "HEAT MAP";

        const title = template.find((entry) => entry.name === "title");
        title.default = "Heat Map";
        const colorMap = template.find((entry) => entry.name === "color_map");
        colorMap.default = "fire";
        const valueMinimum = template.find((entry) => entry.name === "value_min");
        valueMinimum.default = 0;
        const valueMaximum = template.find((entry) => entry.name === "value_max");
        valueMaximum.default = 1;

        const maximumIndex = template.findIndex((entry) => entry.name === "value_max");
        template.splice(maximumIndex + 1, 0,
            {'name':'auto_range', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'include_zero', 'default':"no", 'type':'bool', 'control':'checkbox'}
        );

        const customColorsIndex = template.findIndex((entry) => entry.name === "color_map_colors");
        template.splice(customColorsIndex + 1, 0,
            {'name':'show_color_legend', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'color_legend_width', 'default':14, 'type':'int', 'control':'textedit'},
            {'name':'color_legend_ticks', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'color_legend_decimals', 'default':2, 'type':'int', 'control':'textedit'}
        );
        return template;
    }

    hasVerticalColorLegend()
    {
        return this.parameters.color_map !== "rgb" && this.toBool(this.parameters.show_color_legend);
    }

    getConfiguredColorMap()
    {
        return this.getColorMap(this.parameters.color_map, this.parameters.color_map_colors);
    }

    getColorLegendRange()
    {
        return {
            min:Number(this.parameters.value_min),
            max:Number(this.parameters.value_max)
        };
    }

    getGridMetrics()
    {
        const metrics = super.getGridMetrics();
        if(!metrics || !this.hasVerticalColorLegend())
            return metrics;
        metrics.usableWidth -= this.getVerticalColorLegendSpace();
        if(metrics.usableWidth <= 0)
            return null;
        metrics.cellWidth = metrics.usableWidth / metrics.cols;
        return metrics;
    }

    updateAutomaticRange()
    {
        if(!this.toBool(this.parameters.auto_range) || this.parameters.color_map === "rgb")
            return;

        const values = this.flattenSource(this.getSource('source', []));
        let minimum = Infinity;
        let maximum = -Infinity;
        for(const entry of values)
        {
            const value = Number(entry);
            if(!Number.isFinite(value))
                continue;
            minimum = Math.min(minimum, value);
            maximum = Math.max(maximum, value);
        }
        if(!Number.isFinite(minimum) || !Number.isFinite(maximum))
            return;
        if(this.toBool(this.parameters.include_zero))
        {
            minimum = Math.min(minimum, 0);
            maximum = Math.max(maximum, 0);
        }
        if(minimum === maximum)
        {
            const padding = Math.max(0.5, Math.abs(minimum) * 0.05);
            minimum -= padding;
            maximum += padding;
        }
        this.parameters.value_min = minimum;
        this.parameters.value_max = maximum;
    }

    update()
    {
        this.updateAutomaticRange();
        super.update();
    }
}


webui_widgets.add('webui-widget-heat-map', WebUIWidgetHeatMap);
