/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   cuteime                                                                  *
 *   Copyright (C) 2009-2015 by Tasuku Suzuki <stasuku@gmail.com>            *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Lesser Public License as          *
 *   published by the Free Software Foundation; either version 2 of the      *
 *   License, or (at your option) any later version.                         *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU Lesser General Public License for more details.                     *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this program; if not, write to the                   *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <QDBusConnection>
#include <QtConcurrentRun>

#include <cuteimepreeditmanager.h>

class CuteimePreeditManagerTest : public QObject
{
    Q_OBJECT
public:
    CuteimePreeditManagerTest(QObject *parent = 0)
        : QObject(parent)
    {
        bool ret = QDBusConnection::sessionBus().registerService(CUTEIME_DBUS_SERVICE);
        if (ret) {
            preeditManager = new CuteimePreeditManager(this, CuteimePreeditManager::Server);
            preeditManager->init();
            QStringList args = QCoreApplication::arguments();
            args.removeFirst();
            client = QtConcurrent::run(QProcess::execute, QCoreApplication::applicationFilePath(), args);
        } else {
            preeditManager = new CuteimePreeditManager(this, CuteimePreeditManager::Client);
            preeditManager->init();
        }
    }

    ~CuteimePreeditManagerTest() {
        if (client.isStarted()) {
            client.waitForFinished();
        }
    }

public slots:
    void initTestCase() {
        qDebug() << preeditManager->type();
    }

    void init() {
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            break;
        case CuteimePreeditManager::Client:
            break;
        }
        expected.clear();
        signalReceived = false;
    }

    void cleanup() {
        signalReceived = false;
    }

    void cleanupTestCase() {
        wait();
    }

protected slots:
    void itemChanged(const CuteimePreeditItem &item) {
        QCOMPARE(item.to, expected.value<CuteimePreeditItem>().to);
        QCOMPARE(item.from, expected.value<CuteimePreeditItem>().from);
        QCOMPARE(item.rawString, expected.value<CuteimePreeditItem>().rawString);
        QCOMPARE(item.cursor, expected.value<CuteimePreeditItem>().cursor);
        QCOMPARE(item.selection, expected.value<CuteimePreeditItem>().selection);
        QCOMPARE(item.modified, expected.value<CuteimePreeditItem>().modified);
        signalReceived = true;
    }

    void rectChanged(const QRect &rect) {
        QCOMPARE(rect, expected.toRect());
        signalReceived = true;
    }

#ifndef CUTEIME_NO_GUI
    void fontChanged(const QFont &font) {
        QCOMPARE(font, expected.value<QFont>());
        signalReceived = true;
    }
#endif

    void cursorPositionChanged(int cursorPosition) {
        QCOMPARE(cursorPosition, expected.toInt());
        signalReceived = true;
    }

    void surroundingTextChanged(const QString &surroundingText) {
        QCOMPARE(surroundingText, expected.toString());
        signalReceived = true;
    }

    void currentSelectionChanged(const QString &currentSelection) {
        QCOMPARE(currentSelection, expected.toString());
        signalReceived = true;
    }

    void maximumTextLengthChanged(int maximumTextLength) {
        QCOMPARE(maximumTextLength, expected.toInt());
        signalReceived = true;
    }

private slots:
    void setItem_data() {
        QTest::addColumn<CuteimePreeditItem>("data");

        CuteimePreeditItem item;
        QString alphabet = QString::fromUtf8("abcdefgあいうえお");
        for (int i = 1; i < alphabet.length(); i++) {
            CuteimePreeditItem item;
            item.to.append(alphabet.left(i));
            item.cursor = 0;
            item.selection = 0;
            item.modified = 0;
            QTest::newRow(QString("alphabet(%1)").arg(i).toLocal8Bit().data()) << item;
        }
    }

    void setItem() {
        QFETCH(CuteimePreeditItem, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(itemChanged(CuteimePreeditItem)), this, SLOT(itemChanged(CuteimePreeditItem)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->item().to, data.to);
            QCOMPARE(preeditManager->item().from, data.from);
            QCOMPARE(preeditManager->item().rawString, data.rawString);
            QCOMPARE(preeditManager->item().cursor, data.cursor);
            QCOMPARE(preeditManager->item().selection, data.selection);
            QCOMPARE(preeditManager->item().modified, data.modified);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setItem(data);
            QCOMPARE(preeditManager->item().to, data.to);
            QCOMPARE(preeditManager->item().from, data.from);
            QCOMPARE(preeditManager->item().rawString, data.rawString);
            QCOMPARE(preeditManager->item().cursor, data.cursor);
            QCOMPARE(preeditManager->item().selection, data.selection);
            QCOMPARE(preeditManager->item().modified, data.modified);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(itemChanged(CuteimePreeditItem)), this, SLOT(itemChanged(CuteimePreeditItem)));
    }

    void setRect_data() {
        QTest::addColumn<QRect>("data");

        for (int i = 0; i < 10; i++) {
            QTest::newRow(QString::number(i).toLocal8Bit().data()) << QRect(i, i, i, i);
        }
    }

    void setRect() {
        QFETCH(QRect, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(rectChanged(QRect)), this, SLOT(rectChanged(QRect)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->rect(), data);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setRect(data);
            QCOMPARE(preeditManager->rect(), data);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(rectChanged(QRect)), this, SLOT(rectChanged(QRect)));
    }

#ifndef CUTEIME_NO_GUI
    void setFont_data() {
        QTest::addColumn<QFont>("data");

        QTest::newRow("") << QFont();
        QTest::newRow("Arial") << QFont("Arial");
    }

    void setFont() {
        QFETCH(QFont, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(fontChanged(QFont)), this, SLOT(fontChanged(QFont)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->font(), data);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setFont(data);
            QCOMPARE(preeditManager->font(), data);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(fontChanged(QFont)), this, SLOT(fontChanged(QFont)));
    }
#endif

    void setCursorPosition_data() {
        QTest::addColumn<int>("data");

        for (int i = 0; i < 10; i++) {
            QTest::newRow(QString::number(i).toLocal8Bit().data()) << i;
        }
    }

    void setCursorPosition() {
        QFETCH(int, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(cursorPositionChanged(int)), this, SLOT(cursorPositionChanged(int)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->cursorPosition(), data);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setCursorPosition(data);
            QCOMPARE(preeditManager->cursorPosition(), data);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(cursorPositionChanged(int)), this, SLOT(cursorPositionChanged(int)));
    }

    void setSurroundingText_data() {
        QTest::addColumn<QString>("data");

        for (int i = 0; i < 10; i++) {
            QTest::newRow(QString::number(i).toLocal8Bit().data()) << QString("surrounding text %1").arg(i);
        }
    }

    void setSurroundingText() {
        QFETCH(QString, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(surroundingTextChanged(QString)), this, SLOT(surroundingTextChanged(QString)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->surroundingText(), data);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setSurroundingText(data);
            QCOMPARE(preeditManager->surroundingText(), data);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(surroundingTextChanged(QString)), this, SLOT(surroundingTextChanged(QString)));
    }

    void setCurrentSelection_data() {
        QTest::addColumn<QString>("data");

        for (int i = 0; i < 10; i++) {
            QTest::newRow(QString::number(i).toLocal8Bit().data()) << QString("surrounding text %1").arg(i);
        }
    }

    void setCurrentSelection() {
        QFETCH(QString, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(currentSelectionChanged(QString)), this, SLOT(currentSelectionChanged(QString)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->currentSelection(), data);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setCurrentSelection(data);
            QCOMPARE(preeditManager->currentSelection(), data);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(currentSelectionChanged(QString)), this, SLOT(currentSelectionChanged(QString)));
    }

    void setMaximumTextLength_data() {
        QTest::addColumn<int>("data");

        for (int i = 0; i < 10; i++) {
            QTest::newRow(QString::number(i).toLocal8Bit().data()) << i;
        }
    }

    void setMaximumTextLength() {
        QFETCH(int, data);
        expected = qVariantFromValue(data);
        connect(preeditManager, SIGNAL(maximumTextLengthChanged(int)), this, SLOT(maximumTextLengthChanged(int)));
        switch (preeditManager->type()) {
        case CuteimePreeditManager::Server:
            signalReceived = false;
            wait();
            QCOMPARE(preeditManager->maximumTextLength(), data);
            break;
        case CuteimePreeditManager::Client:
            wait(200);
            signalReceived = false;
            preeditManager->setMaximumTextLength(data);
            QCOMPARE(preeditManager->maximumTextLength(), data);
            wait();
            break;
        }
        disconnect(preeditManager, SIGNAL(maximumTextLengthChanged(int)), this, SLOT(maximumTextLengthChanged(int)));
    }


private:
    bool wait(int timeout = 1000) {
        bool ret = false;
        QTime timer;
        timer.start();
        while (!signalReceived) {
            QTest::qWait(50);
            if (timer.elapsed() > timeout) {
                ret = true;
                break;
            }
        }
        return ret;
    }

private:
    CuteimePreeditManager *preeditManager;
    QFuture<int> client;
    bool signalReceived;
    QVariant expected;
};

QTEST_MAIN(CuteimePreeditManagerTest)

#include "tst_cuteimepreeditmanager.moc"
