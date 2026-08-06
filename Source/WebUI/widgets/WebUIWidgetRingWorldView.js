class WebUIWidgetRingWorldView extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name': "RING WORLD", 'control':'header'},
            {'name':'title', 'default':"Ring World View", 'type':'string', 'control':'textedit'},
            {'name':'stimuli_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'auditory_stimuli_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'distal_stimuli_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'gaze_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'track_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'field_of_view_parameter', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'field_of_view', 'default':120, 'type':'float', 'control':'textedit'},
            {'name':'upper_half_only', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'stimulus_radius', 'default':10.5, 'type':'float', 'control':'textedit'},
            {'name':'scene_background', 'default':"#f7f5ef", 'type':'string', 'control':'textedit'},
            {'name':'ring_color', 'default':"#50545a", 'type':'string', 'control':'textedit'},
            {'name':'gaze_color', 'default':"#20242a", 'type':'string', 'control':'textedit'},
            {'name':'visual_field_color', 'default':"rgba(100, 105, 112, 0.25)", 'type':'string', 'control':'textedit'}
        ];
    }

    init()
    {
        super.init();
        this.stimuli = [];
        this.auditoryStimuli = [];
        this.distalStimuli = [];
        this.auditoryActive = false;
        this.distalActive = false;
        this.gaze = 0;
        this.track = false;
        this.fieldOfView = 120;
    }

    angleInRadians(degrees)
    {
        return Number(degrees) * Math.PI / 180 - Math.PI / 2;
    }

    geometry()
    {
        const stimulusRadius = Math.max(1, Number(this.parameters.stimulus_radius) || 10.5);
        const margin = stimulusRadius + 12;
        const upperHalfOnly = Boolean(this.parameters.upper_half_only);
        const outerRingSpacing = 2 * stimulusRadius + 7;
        const outerRingCount = this.distalActive ? 2 : this.auditoryActive ? 1 : 0;
        const outerRingSpace = outerRingCount * outerRingSpacing;
        const radius = Math.max(1, Math.min(this.width, this.height) / 2 - margin - outerRingSpace);
        return {
            centerX:this.width / 2,
            centerY:this.height / 2,
            radius,
            auditoryRadius:radius + outerRingSpacing,
            distalRadius:radius + 2 * outerRingSpacing,
            stimulusRadius,
            upperHalfOnly
        };
    }

    wrappedAngle(degrees)
    {
        let angle = (Number(degrees) + 180) % 360;
        if(angle < 0)
            angle += 360;
        return angle - 180;
    }

    rgbColor(row)
    {
        const channel = (index) => Math.round(255 * Math.max(0, Math.min(1, Number(row[index]) || 0)));
        return `rgb(${channel(1)}, ${channel(2)}, ${channel(3)})`;
    }

    symbolEnabled(row, column)
    {
        return Number(row[column]) > 0.5;
    }

    drawStimulusSymbols(x, y, radius, row)
    {
        const extent = radius * 0.58;
        const axialExtent = extent * Math.SQRT2;
        const arrowTip = radius * 0.62;
        const arrowBack = arrowTip / 2;
        const arrowHalfHeight = radius * 0.43;

        this.canvas.save();
        this.canvas.translate(x, y);
        this.canvas.strokeStyle = "#111316";
        this.canvas.fillStyle = "#111316";
        this.canvas.lineWidth = Math.max(1.5, radius * 0.16);
        this.canvas.lineCap = "round";
        this.canvas.lineJoin = "round";

        const line = (x1, y1, x2, y2) =>
        {
            this.canvas.beginPath();
            this.canvas.moveTo(x1, y1);
            this.canvas.lineTo(x2, y2);
            this.canvas.stroke();
        };

        if(this.symbolEnabled(row, 4))
            line(-axialExtent, 0, axialExtent, 0);
        if(this.symbolEnabled(row, 5))
            line(0, -axialExtent, 0, axialExtent);
        if(this.symbolEnabled(row, 6))
            line(-extent, -extent, extent, extent);
        if(this.symbolEnabled(row, 7))
            line(-extent, extent, extent, -extent);
        if(this.symbolEnabled(row, 8))
        {
            this.canvas.beginPath();
            this.canvas.moveTo(-arrowTip, 0);
            this.canvas.lineTo(arrowBack, -arrowHalfHeight);
            this.canvas.lineTo(arrowBack, arrowHalfHeight);
            this.canvas.closePath();
            this.canvas.fill();
        }
        if(this.symbolEnabled(row, 9))
        {
            this.canvas.beginPath();
            this.canvas.moveTo(arrowTip, 0);
            this.canvas.lineTo(-arrowBack, -arrowHalfHeight);
            this.canvas.lineTo(-arrowBack, arrowHalfHeight);
            this.canvas.closePath();
            this.canvas.fill();
        }
        if(this.symbolEnabled(row, 10))
        {
            this.canvas.beginPath();
            this.canvas.arc(0, 0, Math.max(3, radius * 0.36), 0, 2 * Math.PI);
            this.canvas.fill();
        }

        this.canvas.restore();
    }

    drawVisualField(geometry)
    {
        const gaze = this.angleInRadians(this.gaze);
        const halfField = Math.max(0, Math.min(360, this.fieldOfView)) * Math.PI / 360;

        this.canvas.save();
        if(geometry.upperHalfOnly)
        {
            this.canvas.beginPath();
            this.canvas.rect(0, 0, this.width, geometry.centerY);
            this.canvas.clip();
        }
        this.canvas.beginPath();
        this.canvas.moveTo(geometry.centerX, geometry.centerY);
        this.canvas.arc(geometry.centerX, geometry.centerY, geometry.radius,
                        gaze - halfField, gaze + halfField);
        this.canvas.closePath();
        this.canvas.fillStyle = this.parameters.visual_field_color;
        this.canvas.fill();
        this.canvas.restore();
    }

    drawRing(geometry)
    {
        this.canvas.beginPath();
        if(geometry.upperHalfOnly)
            this.canvas.arc(geometry.centerX, geometry.centerY, geometry.radius, Math.PI, 2 * Math.PI);
        else
            this.canvas.arc(geometry.centerX, geometry.centerY, geometry.radius, 0, 2 * Math.PI);
        this.canvas.strokeStyle = this.parameters.ring_color;
        this.canvas.lineWidth = 2;
        this.canvas.stroke();
    }

    drawOptionalRing(geometry, radius)
    {
        this.canvas.save();
        this.canvas.setLineDash([5, 5]);
        this.canvas.beginPath();
        if(geometry.upperHalfOnly)
            this.canvas.arc(geometry.centerX, geometry.centerY, radius, Math.PI, 2 * Math.PI);
        else
            this.canvas.arc(geometry.centerX, geometry.centerY, radius, 0, 2 * Math.PI);
        this.canvas.strokeStyle = this.parameters.ring_color;
        this.canvas.lineWidth = 1;
        this.canvas.stroke();
        this.canvas.restore();
    }

    drawGaze(geometry)
    {
        const angle = this.angleInRadians(this.gaze);
        const showGaze = !geometry.upperHalfOnly || Math.abs(this.wrappedAngle(this.gaze)) <= 90;
        this.canvas.save();
        if(showGaze)
        {
            if(this.track)
                this.drawTrackingArrow(geometry, angle);
            else
            {
                this.canvas.setLineDash([7, 5]);
                this.canvas.beginPath();
                this.canvas.moveTo(geometry.centerX, geometry.centerY);
                this.canvas.lineTo(geometry.centerX + geometry.radius * Math.cos(angle),
                                   geometry.centerY + geometry.radius * Math.sin(angle));
                this.canvas.strokeStyle = this.parameters.gaze_color;
                this.canvas.lineWidth = 1.5;
                this.canvas.stroke();
            }
        }

        this.canvas.setLineDash([]);
        this.canvas.beginPath();
        this.canvas.arc(geometry.centerX, geometry.centerY, 4, 0, 2 * Math.PI);
        this.canvas.fillStyle = this.parameters.gaze_color;
        this.canvas.fill();
        this.canvas.restore();
    }

    drawTrackingArrow(geometry, angle)
    {
        const tipRadius = Math.max(0, geometry.radius - geometry.stimulusRadius);
        const tipX = geometry.centerX + tipRadius * Math.cos(angle);
        const tipY = geometry.centerY + tipRadius * Math.sin(angle);
        const headLength = Math.min(12, Math.max(7, geometry.radius * 0.06));
        const headWidth = headLength * 0.55;
        const baseX = tipX - headLength * Math.cos(angle);
        const baseY = tipY - headLength * Math.sin(angle);
        const perpendicularX = -Math.sin(angle);
        const perpendicularY = Math.cos(angle);

        this.canvas.setLineDash([]);
        this.canvas.beginPath();
        this.canvas.moveTo(geometry.centerX, geometry.centerY);
        this.canvas.lineTo(baseX, baseY);
        this.canvas.strokeStyle = this.parameters.gaze_color;
        this.canvas.lineWidth = 2.5;
        this.canvas.stroke();

        this.canvas.beginPath();
        this.canvas.moveTo(tipX, tipY);
        this.canvas.lineTo(baseX + headWidth * perpendicularX,
                           baseY + headWidth * perpendicularY);
        this.canvas.lineTo(baseX - headWidth * perpendicularX,
                           baseY - headWidth * perpendicularY);
        this.canvas.closePath();
        this.canvas.fillStyle = this.parameters.gaze_color;
        this.canvas.fill();
    }

    drawStimulus(geometry, row, ringRadius=geometry.radius)
    {
        if(!Array.isArray(row) || row.length < 4 || !Number.isFinite(Number(row[0])))
            return;
        if(geometry.upperHalfOnly && Math.abs(this.wrappedAngle(row[0])) > 90)
            return;

        const angle = this.angleInRadians(row[0]);
        const x = geometry.centerX + ringRadius * Math.cos(angle);
        const y = geometry.centerY + ringRadius * Math.sin(angle);

        this.canvas.beginPath();
        this.canvas.arc(x, y, geometry.stimulusRadius, 0, 2 * Math.PI);
        this.canvas.fillStyle = this.rgbColor(row);
        this.canvas.fill();
        this.canvas.strokeStyle = "#26292e";
        this.canvas.lineWidth = 1.5;
        this.canvas.stroke();
        this.drawStimulusSymbols(x, y, geometry.stimulusRadius, row);
    }

    drawAuditoryStimulus(geometry, row)
    {
        if(!Array.isArray(row) || row.length < 2 || !Number.isFinite(Number(row[0])))
            return;
        if(geometry.upperHalfOnly && Math.abs(this.wrappedAngle(row[0])) > 90)
            return;

        const intensity = Math.max(0, Math.min(1, Number(row[1]) || 0));
        if(intensity <= 0)
            return;

        const angle = this.angleInRadians(row[0]);
        const x = geometry.centerX + geometry.auditoryRadius * Math.cos(angle);
        const y = geometry.centerY + geometry.auditoryRadius * Math.sin(angle);
        this.canvas.beginPath();
        this.canvas.arc(x, y, geometry.stimulusRadius * intensity, 0, 2 * Math.PI);
        this.canvas.fillStyle = "#111316";
        this.canvas.fill();
    }

    drawScene()
    {
        this.clearCanvas(0, 0);
        this.canvas.fillStyle = this.parameters.scene_background;
        this.canvas.fillRect(0, 0, this.width, this.height);

        const geometry = this.geometry();
        this.drawVisualField(geometry);
        this.drawRing(geometry);
        if(this.auditoryActive)
            this.drawOptionalRing(geometry, geometry.auditoryRadius);
        if(this.distalActive)
            this.drawOptionalRing(geometry, geometry.distalRadius);
        this.drawGaze(geometry);
        for(const row of this.stimuli)
            this.drawStimulus(geometry, row);
        for(const row of this.auditoryStimuli)
            this.drawAuditoryStimulus(geometry, row);
        for(const row of this.distalStimuli)
            this.drawStimulus(geometry, row, geometry.distalRadius);
    }

    update(d)
    {
        this.stimuli = this.matrixRows(this.getSource('stimuli_source', []));
        const auditoryStimuli = this.getSource('auditory_stimuli_source', null);
        const distalStimuli = this.getSource('distal_stimuli_source', null);
        this.auditoryActive = Array.isArray(auditoryStimuli);
        this.distalActive = Array.isArray(distalStimuli);
        this.auditoryStimuli = this.matrixRows(auditoryStimuli);
        this.distalStimuli = this.matrixRows(distalStimuli);
        this.gaze = this.sourceNumber('gaze_source', 0);
        this.track = this.sourceNumber('track_source', 0) == 1;
        this.fieldOfView = this.sourceNumber('field_of_view_parameter',
                                             Number(this.parameters.field_of_view) || 120);
        this.resetCanvasTransform();
        this.drawScene();
    }
}

webui_widgets.add('webui-widget-ringworldview', WebUIWidgetRingWorldView);
