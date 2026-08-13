class WebUIWidgetNavigationHUD extends WebUIWidgetAngularHUD
{
    static template()
    {
        return [
            {'name': "NAVIGATION HUD", 'control':'header'},
            {'name':'title', 'default':"Navigation HUD", 'type':'string', 'control':'textedit'},
            {'name':'heading_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'target_heading_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'position_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'speed_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'angular_velocity_source', 'default':"", 'type':'source', 'control':'textedit'},

            {'name': "SCALE", 'control':'header'},
            {'name':'angle_unit', 'default':"degrees", 'type':'string', 'control':'menu', 'options':"degrees,radians"},
            {'name':'field_of_view', 'default':120, 'type':'float', 'control':'textedit'},
            {'name':'minor_tick_step', 'default':5, 'type':'float', 'control':'textedit'},
            {'name':'major_tick_step', 'default':30, 'type':'float', 'control':'textedit'},
            {'name':'show_cardinals', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'heading_decimals', 'default':0, 'type':'int', 'control':'textedit'},

            {'name': "LAYOUT", 'control':'header'},
            {'name':'safe_left', 'default':24, 'type':'float', 'control':'textedit'},
            {'name':'safe_right', 'default':24, 'type':'float', 'control':'textedit'},
            {'name':'safe_bottom', 'default':16, 'type':'float', 'control':'textedit'},
            {'name':'bottom_band_height', 'default':96, 'type':'float', 'control':'textedit'},
            {'name':'show_telemetry', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'show_units', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'position_decimals', 'default':1, 'type':'int', 'control':'textedit'},
            {'name':'speed_decimals', 'default':2, 'type':'int', 'control':'textedit'},
            {'name':'band_background', 'default':"transparent", 'type':'string', 'control':'menu', 'options':"transparent,gradient,solid"},
            {'name':'band_opacity', 'default':0.5, 'type':'float', 'control':'slider', 'min':0, 'max':1},

            {'name': "STYLE", 'control':'header'},
            {'name':'navigation_color', 'default':'#67c1ff', 'type':'string', 'control':'textedit'},
            {'name':'target_color', 'default':'#e66cff', 'type':'string', 'control':'textedit'},
            {'name':'separator_color', 'default':'rgba(255,255,255,0.45)', 'type':'string', 'control':'textedit'},
            {'name':'outline_color', 'default':'rgba(0,0,0,0.85)', 'type':'string', 'control':'textedit'},
            {'name':'outline_width', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'line_width', 'default':1.5, 'type':'float', 'control':'textedit'},
            {'name':'hud_font', 'default':'13px sans-serif', 'type':'string', 'control':'textedit'},
            {'name':'scale_font', 'default':'12px sans-serif', 'type':'string', 'control':'textedit'},
        ];
    }

    drawBandBackground(width, top, bottom)
    {
        if(this.parameters.band_background === "transparent")
            return;
        const configuredOpacity = Number(this.parameters.band_opacity);
        const opacity = Number.isFinite(configuredOpacity) ? Math.max(0, Math.min(1, configuredOpacity)) : 0.5;
        this.canvas.save();
        if(this.parameters.band_background === "solid")
            this.canvas.fillStyle = `rgba(0,0,0,${opacity})`;
        else
        {
            const gradient = this.canvas.createLinearGradient(0, top, 0, bottom);
            gradient.addColorStop(0, "rgba(0,0,0,0)");
            gradient.addColorStop(1, `rgba(0,0,0,${opacity})`);
            this.canvas.fillStyle = gradient;
        }
        this.canvas.fillRect(0, top, width, bottom - top);
        this.canvas.restore();
    }

    getPosition()
    {
        const source = this.getSource('position_source');
        const x = Number(this.sourceScalar(source, 0));
        const y = Number(this.sourceScalar(source, 1));
        return Number.isFinite(x) && Number.isFinite(y) ? {x, y} : null;
    }

    drawTelemetry(width, top, heading, position, speed, angularVelocity, color)
    {
        if(!this.parameters.show_telemetry)
            return;
        const left = Math.max(4, Number(this.parameters.safe_left) || 0);
        const right = width - Math.max(4, Number(this.parameters.safe_right) || 0);
        const units = this.parameters.show_units;
        const headingText = `${this.formatHUDValue(heading, this.parameters.heading_decimals)}\u00B0`;
        this.drawOutlinedText(`NAV ${headingText}`, left, top - 9, {color, align:"left", baseline:"bottom"});
        if(position)
        {
            const x = this.formatHUDValue(position.x, this.parameters.position_decimals);
            const y = this.formatHUDValue(position.y, this.parameters.position_decimals);
            this.drawOutlinedText(`POS ${x}, ${y}${units ? " m" : ""}`, left, top - 27, {color, align:"left", baseline:"bottom"});
        }
        const rightParts = [];
        if(Number.isFinite(speed))
            rightParts.push(`SPD ${this.formatHUDValue(speed, this.parameters.speed_decimals)}${units ? " m/s" : ""}`);
        if(Number.isFinite(angularVelocity))
            rightParts.push(`TURN ${this.formatHUDValue(angularVelocity, this.parameters.speed_decimals, true)}${units ? "\u00B0/s" : ""}`);
        if(rightParts.length > 0)
            this.drawOutlinedText(rightParts.join("   "), right, top - 9, {color, align:"right", baseline:"bottom"});
    }

    update()
    {
        this.beginCanvasDraw();
        const width = this.format.width;
        const height = this.format.height;
        const heading = this.angleToDegrees(this.sourceValue('heading_source'));
        const target = this.angleToDegrees(this.sourceValue('target_heading_source'));
        const speed = this.sourceValue('speed_source');
        const angularVelocity = this.angleToDegrees(this.sourceValue('angular_velocity_source'));
        const position = this.getPosition();
        const bandHeight = Math.max(48, Number(this.parameters.bottom_band_height) || 96);
        const safeBottom = Math.max(0, Number(this.parameters.safe_bottom) || 0);
        const bottom = height - safeBottom;
        const top = bottom - bandHeight;
        const separatorY = top + bandHeight / 2;
        const baselineY = separatorY - 9;
        const left = Math.max(0, Number(this.parameters.safe_left) || 0);
        const right = width - Math.max(0, Number(this.parameters.safe_right) || 0);
        const color = this.getHUDColor("navigation_color", "#67c1ff");
        const targetColor = this.getHUDColor("target_color", "#e66cff");

        this.drawBandBackground(width, top, bottom);
        if(Number.isFinite(heading))
            this.drawHorizontalTape({
                current:heading,
                target,
                left,
                right,
                baselineY,
                separatorY,
                side:"upper",
                wrap:true,
                period:360,
                fieldOfView:this.parameters.field_of_view,
                minorStep:this.parameters.minor_tick_step,
                majorStep:this.parameters.major_tick_step,
                color,
                targetColor,
                labelFormatter:(value) => this.headingLabel(value, this.parameters.show_cardinals)
            });
        this.drawTelemetry(width, top, heading, position, speed, angularVelocity, color);
    }
}


webui_widgets.add('webui-widget-navigation-hud', WebUIWidgetNavigationHUD);
