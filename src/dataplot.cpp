/***************************************************************************
**                                                                        **
**  FlySight Viewer                                                       **
**  Copyright 2018 Michael Cooper                                         **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>. **
**                                                                        **
****************************************************************************
**  Contact: Michael Cooper                                               **
**  Website: http://flysight.ca/                                          **
****************************************************************************/

#include <QToolTip>

#include "dataplot.h"
#include "mainwindow.h"

DataPlot::DataPlot(QWidget *parent) :
    QCustomPlot(parent),
    mMainWindow(0),
    m_dragging(false),
    m_xAxisType(Time),
    m_cursorValid(false)
{
    // Initialize window
    setMouseTracking(true);
    setCursor(QCursor(Qt::ArrowCursor));

    // Intitialize plot area
    initPlot();

    // Read plot settings
    readSettings();
}

DataPlot::~DataPlot()
{
    // Write plot settings
    writeSettings();

    // Save plot state
    foreach (PlotValue *v, m_yValues)
    {
        v->writeSettings();
    }

    // Delete plots
    while (!m_xValues.isEmpty())
    {
        delete m_xValues.takeLast();
    }

    while (!m_yValues.isEmpty())
    {
        delete m_yValues.takeLast();
    }
}

void DataPlot::initPlot()
{
    m_xValues.append(new PlotTime);
    m_xValues.append(new PlotDistance2D);
    m_xValues.append(new PlotDistance3D);

    foreach (PlotValue *v, m_xValues)
    {
        v->readSettings();
    }

    m_yValues.append(new PlotElevation);
    m_yValues.append(new PlotVerticalSpeed);
    m_yValues.append(new PlotHorizontalSpeed);
    m_yValues.append(new PlotTotalSpeed);
    m_yValues.append(new PlotDiveAngle);
    m_yValues.append(new PlotCurvature);
    m_yValues.append(new PlotGlideRatio);
    m_yValues.append(new PlotHorizontalAccuracy);
    m_yValues.append(new PlotVerticalAccuracy);
    m_yValues.append(new PlotSpeedAccuracy);
    m_yValues.append(new PlotNumberOfSatellites);
    m_yValues.append(new PlotAcceleration);
    m_yValues.append(new PlotTotalEnergy);
    m_yValues.append(new PlotEnergyRate);
    m_yValues.append(new PlotLift);
    m_yValues.append(new PlotDrag);
    m_yValues.append(new PlotCourse);
    m_yValues.append(new PlotCourseRate);
    m_yValues.append(new PlotCourseAccuracy);

    foreach (PlotValue *v, m_yValues)
    {
        v->readSettings();
    }
}

void DataPlot::readSettings()
{
    QSettings settings("FlySight", "Viewer");

    settings.beginGroup("mainWindow");
    m_xAxisType = (XAxisType) settings.value("xAxis", m_xAxisType).toInt();
    settings.endGroup();
}

void DataPlot::writeSettings()
{
    QSettings settings("FlySight", "Viewer");

    settings.beginGroup("mainWindow");
    settings.setValue("xAxis", m_xAxisType);
    settings.endGroup();
}

void DataPlot::mousePressEvent(
        QMouseEvent *event)
{
    if (axisRect()->rect().contains(event->pos()))
    {
        m_tBegin = m_tCursor = xAxis->pixelToCoord(event->pos().x());
        m_yBegin = m_yCursor = event->pos().y();
        m_dragging = true;
        updateCursor();
    }

    QCustomPlot::mousePressEvent(event);
}

void DataPlot::mouseReleaseEvent(
        QMouseEvent *event)
{
    QPoint endPos = event->pos();

    MainWindow::Tool tool = mMainWindow->tool();
    if (m_dragging && tool == MainWindow::Zoom)
    {
        QCPRange range(qMin(m_tBegin,
                            xAxis->pixelToCoord(endPos.x())),
                       qMax(m_tBegin,
                            xAxis->pixelToCoord(endPos.x())));

        setRange(range);
    }
    if (m_dragging && tool == MainWindow::Zero)
    {
        DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(endPos.x()));
        mMainWindow->setZero(dpEnd.t);
    }
    if (m_dragging && tool == MainWindow::Ground)
    {
        DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(endPos.x()));
        mMainWindow->setGround(dpEnd.t);
    }
    if (m_dragging && tool == MainWindow::Course)
    {
        DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(endPos.x()));
        mMainWindow->setCourse(dpEnd.t);
    }

    if (m_dragging)
    {
        m_dragging = false;
        updateCursor();
    }

    QCustomPlot::mouseReleaseEvent(event);
}

void DataPlot::mouseMoveEvent(
        QMouseEvent *event)
{
    m_tCursor = xAxis->pixelToCoord(event->pos().x());
    m_yCursor = event->pos().y();
    m_cursorValid = m_dragging || axisRect()->rect().contains(event->pos());

    MainWindow::Tool tool = mMainWindow->tool();
    if (m_dragging && tool == MainWindow::Pan)
    {
        QCPRange range = xAxis->range();

        double diff = m_tBegin - m_tCursor;
        range = QCPRange(range.lower + diff, range.upper + diff);

        setRange(range);

        m_tCursor = m_tBegin;
        m_yBegin = m_yCursor;
    }

    if (axisRect()->rect().contains(event->pos()))
    {
        if (m_dragging && tool == MainWindow::Measure)
        {
            DataPoint dpStart = interpolateDataX(m_tBegin);
            DataPoint dpEnd = interpolateDataX(m_tCursor);
            mMainWindow->setMark(dpStart.t, dpEnd.t);
        }
        else
        {
            if (mMainWindow->dataSize() == 0)
                return;

            DataPoint dp = interpolateDataX(m_tCursor);
            mMainWindow->setMark(dp.t);
        }
    }
    else
    {
        mMainWindow->clearMark();
        QToolTip::hideText();
    }

    update();

    QCustomPlot::mouseMoveEvent(event);
}

void DataPlot::wheelEvent(
        QWheelEvent *event)
{
    if (axisRect()->rect().contains(event->pos()))
    {
        double multiplier = exp((double) -event->angleDelta().y() / 500);

        double x = xAxis->pixelToCoord(event->pos().x());

        QCPRange range = xAxis->range();

        range = QCPRange(
                    x + (range.lower - x) * multiplier,
                    x + (range.upper - x) * multiplier);

        setRange(range);
    }
}

void DataPlot::leaveEvent(
        QEvent *)
{
    mMainWindow->clearMark();
    m_cursorValid = false;
    update();
}

void DataPlot::setMark(
        double start,
        double end)
{
    DataPoint dpStart = interpolateDataX(start);
    DataPoint dpEnd = interpolateDataX(end);

    if (mMainWindow->dataSize() == 0) return;

    QString status;
    if (dpStart.dateTime.date() == dpEnd.dateTime.date())
    {
        status = QString("<p style='color:black;' align='center'><u>%1 %2.%3 to %4.%5 UTC</u></p>")
                .arg(dpStart.dateTime.toUTC().date().toString(Qt::ISODate))
                .arg(dpStart.dateTime.toUTC().time().toString(Qt::ISODate))
                .arg(QString("%1").arg(dpStart.dateTime.time().msec(), 3, 10, QChar('0')))
                .arg(dpEnd.dateTime.toUTC().time().toString(Qt::ISODate))
                .arg(QString("%1").arg(dpEnd.dateTime.time().msec(), 3, 10, QChar('0')));
    }
    else
    {
        status = QString("<p style='color:black;' align='center'><u>%1 %2.%3 to %4 %5.%6 UTC</u></p>")
                .arg(dpStart.dateTime.toUTC().date().toString(Qt::ISODate))
                .arg(dpStart.dateTime.toUTC().time().toString(Qt::ISODate))
                .arg(QString("%1").arg(dpStart.dateTime.time().msec(), 3, 10, QChar('0')))
                .arg(dpEnd.dateTime.toUTC().date().toString(Qt::ISODate))
                .arg(dpEnd.dateTime.toUTC().time().toString(Qt::ISODate))
                .arg(QString("%1").arg(dpEnd.dateTime.time().msec(), 3, 10, QChar('0')));
    }

    status += QString("<table width='400'>");

    status += QString("<tr style='color:black;'><td></td><td><u>Value</u></td><td><u>Change</u></td><td><u>Min/Avg/Max</u></td></tr>");

    double change = m_xValues[Time]->value(dpEnd, mMainWindow->units())
            - m_xValues[Time]->value(dpStart, mMainWindow->units());
    status += QString("<tr style='color:black;'><td>%1</td><td>%2</td><td>(%3%4)</td></tr>")
            .arg(m_xValues[Time]->title(mMainWindow->units()))
            .arg(m_xValues[Time]->value(dpEnd, mMainWindow->units()))
            .arg(change < 0 ? "" : "+")
            .arg(change);

    change = m_xValues[Distance2D]->value(dpEnd, mMainWindow->units())
            - m_xValues[Distance2D]->value(dpStart, mMainWindow->units());
    status += QString("<tr style='color:black;'><td>%1</td><td>%2</td><td>(%3%4)</td></tr>")
            .arg(m_xValues[Distance2D]->title(mMainWindow->units()))
            .arg(m_xValues[Distance2D]->value(dpEnd, mMainWindow->units()))
            .arg(change < 0 ? "" : "+")
            .arg(change);

    if (m_xAxisType != Time && m_xAxisType != Distance2D)
    {
        change = xValue()->value(dpEnd, mMainWindow->units())
                - xValue()->value(dpStart, mMainWindow->units());
        status += QString("<tr style='color:black;'><td>%1</td><td>%2</td><td>(%3%4)</td></tr>")
                .arg(xValue()->title(mMainWindow->units()))
                .arg(xValue()->value(dpEnd, mMainWindow->units()))
                .arg(change < 0 ? "" : "+")
                .arg(change);
    }

    double low  = qMin(start, end);
    double high = qMax(start, end);

    DataPoint dpLow = interpolateDataX(low);
    DataPoint dpHigh = interpolateDataX(high);

    const int jMin = findIndexAboveX(low);
    const int jMax = findIndexBelowX(high);

    const DataPoint &dpMin = mMainWindow->dataPoint(jMin);
    const DataPoint &dpMax = mMainWindow->dataPoint(jMax);

    for (int i = 0; i < yaLast; ++i)
    {
        if (yValue(i)->visible())
        {
            double dx = m_xValues[Time]->value(dpMin, mMainWindow->units())
                    - m_xValues[Time]->value(dpLow, mMainWindow->units());
            double avg = (yValue(i)->value(dpMin, mMainWindow->units())
                    + yValue(i)->value(dpLow, mMainWindow->units())) / 2;

            double sum = avg * fabs(dx);
            double dxSum = fabs(dx);

            double min = yValue(i)->value(dpLow, mMainWindow->units());
            double max = min;

            for (int j = jMin ; j < jMax ; ++ j)
            {
                const DataPoint &dp1 = mMainWindow->dataPoint(j);
                const DataPoint &dp2 = mMainWindow->dataPoint(j + 1);

                dx = m_xValues[Time]->value(dp2, mMainWindow->units())
                        - m_xValues[Time]->value(dp1, mMainWindow->units());
                avg = (yValue(i)->value(dp2, mMainWindow->units())
                        + yValue(i)->value(dp1, mMainWindow->units())) / 2;

                sum += avg * fabs(dx) ;
                dxSum += fabs(dx);

                min = qMin(min, yValue(i)->value(dp1, mMainWindow->units()));
                max = qMax(max, yValue(i)->value(dp1, mMainWindow->units()));
            }

            min = qMin(min, yValue(i)->value(dpMax, mMainWindow->units()));
            max = qMax(max, yValue(i)->value(dpMax, mMainWindow->units()));

            dx = m_xValues[Time]->value(dpHigh, mMainWindow->units())
                    - m_xValues[Time]->value(dpMax, mMainWindow->units());
            avg = (yValue(i)->value(dpHigh, mMainWindow->units())
                    + yValue(i)->value(dpMax, mMainWindow->units())) / 2;

            sum += avg * fabs(dx) ;
            dxSum += fabs(dx);

            min = qMin(min, yValue(i)->value(dpHigh, mMainWindow->units()));
            max = qMax(max, yValue(i)->value(dpHigh, mMainWindow->units()));

            change = yValue(i)->value(dpEnd, mMainWindow->units())
                    - yValue(i)->value(dpStart, mMainWindow->units());
            status += QString("<tr style='color:%5;'><td>%1</td><td>%2</td><td>(%3%4)</td><td>[%6/%7/%8]</td></tr>")
                    .arg(yValue(i)->title(mMainWindow->units()))
                    .arg(yValue(i)->value(dpEnd, mMainWindow->units()))
                    .arg(change < 0 ? "" : "+")
                    .arg(change)
                    .arg(yValue(i)->color().name())
                    .arg(min)
                    .arg(sum / dxSum)
                    .arg(max);
        }
    }

    status += QString("</table>");

    QToolTip::showText(QCursor::pos(), status);
}

void DataPlot::setMark(
        double mark)
{
    if (mMainWindow->dataSize() == 0) return;

    DataPoint dp = interpolateDataX(mark);

    QString status;
    status = QString("<table width='300'>");

    status += QString("<tr style='color:black;'><td align='center'><u>%1 %2.%3 UTC</u></td></tr>")
            .arg(dp.dateTime.toUTC().date().toString(Qt::ISODate))
            .arg(dp.dateTime.toUTC().time().toString(Qt::ISODate))
            .arg(QString("%1").arg(dp.dateTime.time().msec(), 3, 10, QChar('0')));

    status += QString("<tr style='color:black;'><td align='center'><u>(%1 deg, %2 deg, %3 m)</u></td></tr>")
            .arg(dp.lat, 0, 'f', 7)
            .arg(dp.lon, 0, 'f', 7)
            .arg(dp.hMSL, 0, 'f', 3);

    status += QString("</table><table width='300'>");

    status += QString("<tr style='color:black;'><td>%1</td><td>%2</td></tr>")
            .arg(m_xValues[Time]->title(mMainWindow->units()))
            .arg(m_xValues[Time]->value(dp, mMainWindow->units()));

    status += QString("<tr style='color:black;'><td>%1</td><td>%2</td></tr>")
            .arg(m_xValues[Distance2D]->title(mMainWindow->units()))
            .arg(m_xValues[Distance2D]->value(dp, mMainWindow->units()));

    if (m_xAxisType != Time && m_xAxisType != Distance2D)
    {
        status += QString("<tr style='color:black;'><td>%1</td><td>%2</td></tr>")
                .arg(xValue()->title(mMainWindow->units()))
                .arg(xValue()->value(dp, mMainWindow->units()));
    }

    for (int i = 0; i < yaLast; ++i)
    {
        if (yValue(i)->visible())
        {
            status += QString("<tr style='color:%3;'><td>%1</td><td>%2</td></tr>")
                    .arg(yValue(i)->title(mMainWindow->units()))
                    .arg(yValue(i)->value(dp, mMainWindow->units()))
                    .arg(yValue(i)->color().name());
        }
    }

    status += QString("</table>");

    QToolTip::showText(QCursor::pos(), status);
}

void DataPlot::setRange(
        const QCPRange &range)
{
    DataPoint dpLower = interpolateDataX(range.lower);
    DataPoint dpUpper = interpolateDataX(range.upper);

    mMainWindow->setRange(dpLower.t, dpUpper.t);
}

void DataPlot::updateYRanges()
{
    const QCPRange &range = xAxis->range();

    int k = 0;
    for (int j = 0; j < yaLast; ++j)
    {
        if (!yValue(j)->visible()) continue;

        double yMin, yMax;
        bool first = true;

        for (int i = 0; i < mMainWindow->dataSize(); ++i)
        {
            const DataPoint &dp = mMainWindow->dataPoint(i);

            if (range.contains(xValue()->value(dp, mMainWindow->units())))
            {
                double y = yValue(j)->value(dp, mMainWindow->units());

                if (first)
                {
                    yMin = yMax = y;
                    first = false;
                }
                else
                {
                    if (y < yMin) yMin = y;
                    if (y > yMax) yMax = y;
                }
            }
        }

        if (yValue(j)->hasOptimal())
        {
            for (int i = 0; i < mMainWindow->optimalSize(); ++i)
            {
                const DataPoint &dp = mMainWindow->optimalPoint(i);

                if (range.contains(xValue()->value(dp, mMainWindow->units())))
                {
                    double y = yValue(j)->value(dp, mMainWindow->units());

                    if (first)
                    {
                        yMin = yMax = y;
                        first = false;
                    }
                    else
                    {
                        if (y < yMin) yMin = y;
                        if (y > yMax) yMax = y;
                    }
                }
            }
        }

        if (!first)
        {
            const double factor = yValue(j)->factor(mMainWindow->units());
            axisRect()->axis(QCPAxis::atLeft, k++)->setRange(
                        yValue(j)->useMinimum() ? yValue(j)->minimum() * factor : yMin,
                        yValue(j)->useMaximum() ? yValue(j)->maximum() * factor : yMax);
        }
    }
}

void DataPlot::updatePlot()
{
    clearPlottables();
    clearItems();

    xAxis->setLabel(xValue()->title(mMainWindow->units()));

    // Remove all axes
    while (axisRect()->axisCount(QCPAxis::atLeft) > 0)
    {
        axisRect()->removeAxis(axisRect()->axis(QCPAxis::atLeft, 0));
    }

    // Add axes for visible plots
    for (int j = 0; j < yaLast; ++j)
    {
        if (!yValue(j)->visible()) continue;
        yValue(j)->addAxis(this, mMainWindow->units());
    }

    // Return now if plot empty
    if (mMainWindow->dataSize() == 0) return;

    // Get plot range
    DataPoint dpLower = mMainWindow->interpolateDataT(mMainWindow->rangeLower());
    DataPoint dpUpper = mMainWindow->interpolateDataT(mMainWindow->rangeUpper());

    QVector< double > x;
    for (int i = 0; i < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp = mMainWindow->dataPoint(i);
        x.append(xValue()->value(dp, mMainWindow->units()));
    }

    // Draw plots
    for (int j = 0; j < yaLast; ++j)
    {
        if (!yValue(j)->visible()) continue;

        QVector< double > y;
        for (int i = 0; i < mMainWindow->dataSize(); ++i)
        {
            const DataPoint &dp = mMainWindow->dataPoint(i);
            y.append(yValue(j)->value(dp, mMainWindow->units()));
        }

        QCPAxis *axis = yValue(j)->axis();
        QCPGraph *graph = addGraph(
                    axisRect()->axis(QCPAxis::atBottom),
                    axis);
        graph->setData(x, y);
        graph->setPen(QPen(yValue(j)->color(), mMainWindow->lineThickness()));

        if (yValue(j)->hasOptimal())
        {
            QVector< double > xOptimal, yOptimal;
            for (int i = 0; i < mMainWindow->optimalSize(); ++i)
            {
                const DataPoint &dp = mMainWindow->optimalPoint(i);
                xOptimal.append(xValue()->value(dp, mMainWindow->units()));
                yOptimal.append(yValue(j)->value(dp, mMainWindow->units()));
            }

            QCPGraph *graph = addGraph(
                        axisRect()->axis(QCPAxis::atBottom),
                        axis);
            graph->setData(xOptimal, yOptimal);
            graph->setPen(QPen(QBrush(yValue(j)->color()), mMainWindow->lineThickness(), Qt::DotLine));
        }
    }

    if (mMainWindow->windAdjustment())
    {
        // Add label to indicate wind correction
        QCPItemText *textLabel = new QCPItemText(this);

        textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
        textLabel->setTextAlignment(Qt::AlignRight);
        textLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
        textLabel->position->setCoords(1, 0);
        textLabel->setBrush(QBrush(Qt::red));
        textLabel->setColor(Qt::white);
        textLabel->setText(tr("Results are adjusted for wind"));
        textLabel->setFont(QFont(font().family(), font().pointSize(), QFont::Black));
        textLabel->setPadding(QMargins(2, 2, 2, 2));
    }

    updateRange();
}

void DataPlot::updateRange()
{
    if (mMainWindow->dataSize() == 0) return;

    // Get plot range
    DataPoint dpLower = mMainWindow->interpolateDataT(mMainWindow->rangeLower());
    DataPoint dpUpper = mMainWindow->interpolateDataT(mMainWindow->rangeUpper());

    const double xMin = xValue()->value(dpLower, mMainWindow->units());
    const double xMax = xValue()->value(dpUpper, mMainWindow->units());

    // Set x-axis range
    xAxis->setRange(QCPRange(xMin, xMax));

    // Set y-axis ranges
    updateYRanges();

    // Draw annotations on plot background
    mMainWindow->prepareDataPlot(this);

    // Update cursors
    updateCursor();
}

void DataPlot::updateCursor()
{
    setCurrentLayer("overlay");

    foreach (QCPLayerable *l, currentLayer()->children())
    {
        if (qobject_cast<QCPAbstractPlottable*>(l))
        {
            removePlottable((QCPAbstractPlottable*) l);
        }
        else if (qobject_cast<QCPAbstractItem*>(l))
        {
            removeItem((QCPAbstractItem*) l);
        }
    }

    if (mMainWindow->markActive())
    {
        // Draw marks
        const DataPoint &dpEnd = mMainWindow->interpolateDataT(mMainWindow->markEnd());

        QVector< double > xMark, yMark;
        xMark.append(xValue()->value(dpEnd, mMainWindow->units()));

        for (int j = 0; j < yaLast; ++j)
        {
            if (!yValue(j)->visible()) continue;

            yMark.clear();
            yMark.append(yValue(j)->value(dpEnd, mMainWindow->units()));

            QCPAxis *axis = yValue(j)->axis();
            QCPGraph *graph = addGraph(xAxis, axis);

            graph->setData(xMark, yMark);
            graph->setPen(QPen(Qt::black, mMainWindow->lineThickness()));
            graph->setLineStyle(QCPGraph::lsNone);
            graph->setScatterStyle(QCPScatterStyle::ssDisc);
        }

        // Update hover text
        double xCursor = xAxis->coordToPixel(m_tCursor);
        if (axisRect()->rect().contains(xCursor, m_yCursor))
        {
            MainWindow::Tool tool = mMainWindow->tool();
            if (m_dragging && tool == MainWindow::Measure)
            {
                setMark(m_tBegin, m_tCursor);
            }
            else
            {
                setMark(m_tCursor);
            }
        }
    }
    else
    {
        QToolTip::hideText();
    }

    MainWindow::Tool tool = mMainWindow->tool();
    if (!m_cursorValid)
    {
        // Draw nothing
    }
    else if (m_dragging && (tool == MainWindow::Zoom || tool == MainWindow::Measure))
    {
        // Draw shading
        QCPItemRect *rect = new QCPItemRect(this);

        rect->setPen(Qt::NoPen);
        rect->setBrush(QColor(181, 217, 42, 64));

        rect->topLeft->setType(QCPItemPosition::ptPlotCoords);
        rect->topLeft->setAxes(xAxis, yAxis);
        rect->topLeft->setCoords(m_tBegin, yAxis->range().upper);

        rect->bottomRight->setType(QCPItemPosition::ptPlotCoords);
        rect->bottomRight->setAxes(xAxis, yAxis);
        rect->bottomRight->setCoords(m_tCursor, yAxis->range().lower);

        QCPItemLine *line = new QCPItemLine(this);
        line->setPen(QPen(Qt::black));
        line->start->setCoords(m_tBegin, yAxis->range().lower);
        line->end->setCoords(m_tBegin, yAxis->range().upper);

        line = new QCPItemLine(this);
        line->setPen(QPen(Qt::black));
        line->start->setCoords(m_tCursor, yAxis->range().lower);
        line->end->setCoords(m_tCursor, yAxis->range().upper);
    }
    else
    {
        // Draw crosshairs
        QCPItemLine *line = new QCPItemLine(this);
        line->setPen(QPen(Qt::black));
        line->start->setCoords(xAxis->range().lower, yAxis->pixelToCoord(m_yCursor));
        line->end->setCoords(xAxis->range().upper, yAxis->pixelToCoord(m_yCursor));

        line = new QCPItemLine(this);
        line->setPen(QPen(Qt::black));
        line->start->setCoords(m_tCursor, yAxis->range().lower);
        line->end->setCoords(m_tCursor, yAxis->range().upper);
    }

    setCurrentLayer("main");

    replot();
}

DataPoint DataPlot::interpolateDataX(
        double x)
{
    const int i1 = findIndexBelowX(x);
    const int i2 = findIndexAboveX(x);

    if (i1 < 0)
    {
        return mMainWindow->dataPoint(0);
    }
    else if (i2 >= mMainWindow->dataSize())
    {
        return mMainWindow->dataPoint(mMainWindow->dataSize() - 1);
    }
    else
    {
        const DataPoint &dp1 = mMainWindow->dataPoint(i1);
        const DataPoint &dp2 = mMainWindow->dataPoint(i2);
        const double x1 = xValue()->value(dp1, mMainWindow->units());
        const double x2 = xValue()->value(dp2, mMainWindow->units());
        return DataPoint::interpolate(dp1, dp2, (x - x1) / (x2 - x1));
    }
}

int DataPlot::findIndexBelowX(
        double x)
{
    int below = -1;
    int above = mMainWindow->dataSize();

    while (below + 1 != above)
    {
        int mid = (below + above) / 2;
        const DataPoint &dp = mMainWindow->dataPoint(mid);

        if (xValue()->value(dp, mMainWindow->units()) < x) below = mid;
        else                                               above = mid;
    }

    return below;
}

int DataPlot::findIndexAboveX(
        double x)
{
    int below = -1;
    int above = mMainWindow->dataSize();

    while (below + 1 != above)
    {
        int mid = (below + above) / 2;
        const DataPoint &dp = mMainWindow->dataPoint(mid);

        if (xValue()->value(dp, mMainWindow->units()) > x) above = mid;
        else                                               below = mid;
    }

    return above;
}

void DataPlot::togglePlot(
        YAxisType plot)
{
    m_yValues[plot]->setVisible(!m_yValues[plot]->visible());
    updatePlot();
}

void DataPlot::setXAxisType(
        XAxisType xAxisType)
{
    const QCPRange &range = xAxis->range();

    DataPoint dpLower = interpolateDataX(range.lower);
    DataPoint dpUpper = interpolateDataX(range.upper);

    DataPoint dpCursor = interpolateDataX(m_tCursor);
    DataPoint dpBegin = interpolateDataX(m_tBegin);

    m_xAxisType = xAxisType;

    xAxis->setRange(QCPRange(xValue()->value(dpLower, mMainWindow->units()),
                             xValue()->value(dpUpper, mMainWindow->units())));

    m_tCursor = xValue()->value(dpCursor, mMainWindow->units());
    m_tBegin = xValue()->value(dpBegin, mMainWindow->units());

    updatePlot();
}
