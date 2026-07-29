class WebUIWidgetPanTiltHUD extends WebUIWidgetAngularHUD
{
    static template()
    {
        return [
            {'name': "PAN-TILT HUD", 'control':'header'},
            {'name':'title', 'default':"Pan-Tilt HUD", 'type':'string', 'control':'textedit'},
            {'name':'pan_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'tilt_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'target_pan_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'target_tilt_source', 'default':"", 'type':'source', 'control':'textedit'},

            {'name': "PAN SCALE", 'control':'header'},
            {'name':'angle_unit', 'default':"degrees", 'type':'string', 'control':'menu', 'options':"degrees,radians"},
            {'name':'pan_min', 'default':-90, 'type':'float', 'control':'textedit'},
            {'name':'pan_max', 'default':90, 'type':'float', 'control':'textedit'},
            {'name':'pan_field_of_view', 'default':120, 'type':'float', 'control':'textedit'},
            {'name':'pan_minor_tick_step', 'default':5, 'type':'float', 'control':'textedit'},
            {'name':'pan_major_tick_step', 'default':30, 'type':'float', 'control':'textedit'},
            {'name':'wrap_pan', 'default':"no", 'type':'bool', 'control':'checkbox'},

            {'name': "TILT SCALE", 'control':'header'},
            {'name':'tilt_min', 'default':-45, 'type':'float', 'control':'textedit'},
            {'name':'tilt_max', 'default':45, 'type':'float', 'control':'textedit'},
            {'name':'tilt_field_of_view', 'default':60, 'type':'float', 'control':'textedit'},
            {'name':'tilt_minor_tick_step', 'default':5, 'type':'float', 'control':'textedit'},
            {'name':'tilt_major_tick_step', 'default':15, 'type':'float', 'control':'textedit'},

            {'name': "LAYOUT", 'control':'header'},
            {'name':'safe_left', 'default':24, 'type':'float', 'control':'textedit'},
            {'name':'safe_right', 'default':24, 'type':'float', 'control':'textedit'},
            {'name':'safe_top', 'default':24, 'type':'float', 'control':'textedit'},
            {'name':'safe_bottom', 'default':16, 'type':'float', 'control':'textedit'},
            {'name':'bottom_band_height', 'default':96, 'type':'float', 'control':'textedit'},
            {'name':'show_tilt_scale', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'show_reticle', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'show_readout', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'angle_decimals', 'default':1, 'type':'int', 'control':'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'head_color', 'default':'#ffc857', 'type':'string', 'control':'textedit'},
            {'name':'target_color', 'default':'#e66cff', 'type':'string', 'control':'textedit'},
            {'name':'separator_color', 'default':'rgba(255,255,255,0.45)', 'type':'string', 'control':'textedit'},
            {'name':'outline_color', 'default':'rgba(0,0,0,0.85)', 'type':'string', 'control':'textedit'},
            {'name':'outline_width', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'line_width', 'default':1.5, 'type':'float', 'control':'textedit'},
            {'name':'hud_font', 'default':'13px sans-serif', 'type':'string', 'control':'textedit'},
            {'name':'scale_font', 'default':'12px sans-serif', 'type':'string', 'control':'textedit'},
        ];
    }

    drawReadout(width, top, pan, tilt, color)
    {
        if(!this.parameters.show_readout)
            return;
        const center = width / 2;
        const panText = this.formatHUDValue(pan, this.parameters.angle_decimals, true);
        const tiltText = this.formatHUDValue(tilt, this.parameters.angle_decimals, true);
        this.drawOutlinedText(`PAN ${panText}°   TILT ${tiltText}°`, center, top - 9, {
            color,
            align:"center",
            baseline:"bottom"
        });
    }

    update()
    {
        this.beginCanvasDraw();
        const width = this.format.width;
        const height = this.format.height;
        const pan = this.angleToDegrees(this.sourceValue('pan_source'));
        const tilt = this.angleToDegrees(this.sourceValue('tilt_source'));
        const targetPan = this.angleToDegrees(this.sourceValue('target_pan_source'));
        const targetTilt = this.angleToDegrees(this.sourceValue('target_tilt_source'));
        const bandHeight = Math.max(48, Number(this.parameters.bottom_band_height) || 96);
        const safeBottom = Math.max(0, Number(this.parameters.safe_bottom) || 0);
        const bottom = height - safeBottom;
        const top = bottom - bandHeight;
        const separatorY = top + bandHeight / 2;
        const baselineY = separatorY + 9;
        const safeLeft = Math.max(0, Number(this.parameters.safe_left) || 0);
        const safeRight = Math.max(0, Number(this.parameters.safe_right) || 0);
        const safeTop = Math.max(0, Number(this.parameters.safe_top) || 0);
        const color = this.getHUDColor("head_color", "#ffc857");
        const targetColor = this.getHUDColor("target_color", "#e66cff");
        const panMinimum = Number(this.parameters.pan_min);
        const panMaximum = Number(this.parameters.pan_max);

        if(Number.isFinite(pan))
            this.drawHorizontalTape({
                current:pan,
                target:targetPan,
                left:safeLeft,
                right:width - safeRight,
                baselineY,
                separatorY,
                side:"lower",
                wrap:this.parameters.wrap_pan,
                period:360,
                minimum:Number.isFinite(panMinimum) ? panMinimum : -90,
                maximum:Number.isFinite(panMaximum) ? panMaximum : 90,
                fieldOfView:this.parameters.pan_field_of_view,
                minorStep:this.parameters.pan_minor_tick_step,
                majorStep:this.parameters.pan_major_tick_step,
                color,
                targetColor,
                labelFormatter:(value) => this.panLabel(value)
            });

        if(this.parameters.show_tilt_scale && Number.isFinite(tilt))
        {
            const tiltX = width - safeRight - 10;
            const tiltBottom = Math.max(safeTop + 40, top - 20);
            this.drawVerticalTape({
                current:tilt,
                target:targetTilt,
                top:safeTop,
                bottom:tiltBottom,
                x:tiltX,
                minimum:Number(this.parameters.tilt_min),
                maximum:Number(this.parameters.tilt_max),
                fieldOfView:this.parameters.tilt_field_of_view,
                minorStep:this.parameters.tilt_minor_tick_step,
                majorStep:this.parameters.tilt_major_tick_step,
                color,
                targetColor
            });
            this.drawOutlinedText("TILT", tiltX, safeTop - 7, {color, align:"center", baseline:"bottom"});
        }

        if(this.parameters.show_reticle)
        {
            const reticleRight = width - safeRight - (this.parameters.show_tilt_scale ? 64 : 0);
            const reticleCenterX = (safeLeft + reticleRight) / 2;
            const reticleCenterY = (safeTop + Math.max(safeTop + 40, top - 20)) / 2;
            const panError = Number.isFinite(targetPan) && Number.isFinite(pan) ? targetPan - pan : null;
            const tiltError = Number.isFinite(targetTilt) && Number.isFinite(tilt) ? targetTilt - tilt : null;
            this.drawReticle(
                reticleCenterX,
                reticleCenterY,
                panError,
                tiltError,
                this.parameters.pan_field_of_view,
                this.parameters.tilt_field_of_view,
                Math.max(1, reticleRight - safeLeft),
                Math.max(1, top - safeTop - 40),
                color,
                targetColor
            );
        }
        this.drawReadout(width, top, pan, tilt, color);
    }
}


webui_widgets.add('webui-widget-pan-tilt-hud', WebUIWidgetPanTiltHUD);
