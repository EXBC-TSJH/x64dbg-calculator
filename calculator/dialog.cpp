#include "dialog.h"
#include "ui_dialog.h"
#include "ValidateExpressionThread.h"
#include "expression.h"

#include <cstdio>
#include <windows.h>

#include <QDebug>

static duint bswap(duint value)
{
    duint result = 0;
    for(size_t i = 0; i < sizeof(value); i++)
        ((unsigned char*)&result)[sizeof(value) - i - 1] = ((unsigned char*)&value)[i];
    return result;
}

QString CalculatorDialog::inFormat(const duint val, CalculatorDialog::Format format) const
{
    switch(format)
    {
    default:
    case Format::Hex:
        return QString("%1").arg(val, 1, 16, QChar('0')).toUpper();
    case Format::SignedDec:
        return QString("%1").arg((dsint)val);
    case Format::UnsignedDec:
        return QString("%1").arg(val);
    case Format::Binary:
    {
        QString binary = QString("%1").arg(val, 8 * sizeof(duint), 2, QChar('0')).toUpper();
        QString ans = "";
        for(int i = 0; i < (int)sizeof(duint) * 8; i++)
        {
            if((i % 4 == 0) && (i != 0))
                ans += " ";
            ans += binary[i];
        }
        return ans;
    }
    case Format::Octal:
        return QString("%1").arg(val, 1, 8, QChar('0')).toUpper();
    case Format::Bytes:
        return QString("%1").arg(bswap(val), 2 * sizeof(duint), 16, QChar('0')).toUpper();
    }
}

CalculatorDialog::CalculatorDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CalculatorDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    emit validAddress(false);

    ui->txtBytes->setInputMask(QString("HH").repeated(sizeof(duint)));
    ui->txtBin->setInputMask(QString("bbbb ").repeated(sizeof(duint) * 2).trimmed());

    ui->txtExpression->setText("0");
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();

    mValidateThread = new ValidateExpressionThread(this);
    mValidateThread->setOnExpressionChangedCallback(std::bind(&CalculatorDialog::validateExpression, this, std::placeholders::_1));

    connect(mValidateThread, SIGNAL(expressionChanged(bool, bool, dsint)), this, SLOT(expressionChanged(bool, bool, dsint)));
    connect(ui->txtExpression, SIGNAL(textChanged(QString)), mValidateThread, SLOT(textChanged(QString)));
}

CalculatorDialog::~CalculatorDialog()
{
    mValidateThread->stop();
    mValidateThread->wait();
    delete ui;
}

void CalculatorDialog::validateExpression(QString expression)
{
    if(expression == "")
    {
        this->mValidateThread->emitExpressionChanged(true, true, 0);
        return;
    }

    duint value = expression_calculation(std::string(expression.toUtf8().constData()));

    if(*get_err() != 0)
    {
        this->mValidateThread->emitExpressionChanged(false, false, value);
        return;
    }

    this->mValidateThread->emitExpressionChanged(true, true, value);
}

void CalculatorDialog::setExpressionFocus()
{
    ui->txtExpression->selectAll();
    ui->txtExpression->setFocus();
}

void CalculatorDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start();
}

void CalculatorDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->stop();
    mValidateThread->wait();
}

void CalculatorDialog::expressionChanged(bool validExpression, bool validPointer, dsint value)
{
    if(!validExpression)
    {
        ui->txtHex->setText("");
        ui->txtSignedDec->setText("");
        ui->txtUnsignedDec->setText("");
        ui->txtOct->setText("");
        ui->txtBytes->setText("");
        ui->txtBin->setText("");
        ui->txtAscii->setText("");
        ui->txtUnicode->setText("");

        ui->txtExpression->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        emit validAddress(false);
    }
    else
    {
        ui->txtExpression->setStyleSheet("background-color: rgb(49, 49, 49);");
        ui->txtHex->setText(inFormat(value, Format::Hex));
        ui->txtSignedDec->setText(inFormat(value, Format::SignedDec));
        ui->txtUnsignedDec->setText(inFormat(value, Format::UnsignedDec));
        ui->txtOct->setText(inFormat(value, Format::Octal));

        {
            int cursorpos = ui->txtBytes->cursorPosition();
            ui->txtBytes->setText(inFormat(value, Format::Bytes));
            ui->txtBytes->setCursorPosition(cursorpos);
        }

        {

            int cursorpos = ui->txtBin->cursorPosition();
            ui->txtBin->setText(inFormat(value, Format::Binary));
            ui->txtBin->setCursorPosition(cursorpos);
        }

        if(value == (value & 0xFF))
        {
            QChar c((ushort)value);
            if(c.isPrint())
                ui->txtAscii->setText(QString(c));
            else
                ui->txtAscii->setText("???");
        }
        else
            ui->txtAscii->setText("???");

        ui->txtAscii->setCursorPosition(0);
        ui->txtAscii->selectAll();
        if((value == (value & 0xFFFF))) //UNICODE?
        {
            QChar c = QChar((ushort)value);
            if(c.isPrint())
                ui->txtUnicode->setText(QString(c));
            else
                ui->txtUnicode->setText("????");
        }
        else
        {
            ui->txtUnicode->setText("????");
        }
        ui->txtUnicode->setCursorPosition(0);
        ui->txtUnicode->selectAll();

        emit validAddress(validPointer);
    }
}

void CalculatorDialog::on_txtExpression_textChanged(const QString & arg1)
{
    Q_UNUSED(arg1);
    ui->txtHex->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtSignedDec->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtUnsignedDec->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtOct->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtBytes->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtBin->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtAscii->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtUnicode->setStyleSheet("background-color: rgb(49, 49, 49);");
    emit validAddress(false);
}

void CalculatorDialog::on_txtHex_textEdited(const QString & arg1)
{
    bool ok = false;
    ULONGLONG val = arg1.toULongLong(&ok, 16);
    if(!ok)
    {
        ui->txtHex->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        return;
    }
    ui->txtHex->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 10, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtSignedDec_textEdited(const QString & arg1)
{
    bool ok = false;
    LONGLONG val = arg1.toLongLong(&ok, 10);
    if(!ok)
    {
        ui->txtUnsignedDec->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        return;
    }
    ui->txtUnsignedDec->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 10, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtUnsignedDec_textEdited(const QString & arg1)
{
    bool ok = false;
    LONGLONG val = arg1.toULongLong(&ok, 10);
    if(!ok)
    {
        ui->txtUnsignedDec->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        return;
    }
    ui->txtUnsignedDec->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 10, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtOct_textEdited(const QString & arg1)
{
    bool ok = false;
    ULONGLONG val = arg1.toULongLong(&ok, 8);
    if(!ok)
    {
        ui->txtOct->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        return;
    }
    ui->txtOct->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 10, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtBytes_textEdited(const QString & arg1)
{
    bool ok = false;
    QString text = arg1;
    text = text.leftJustified(sizeof(duint) * 2, '0', true);
    ULONGLONG val = text.toULongLong(&ok, 16);
    if(!ok)
    {
        ui->txtBytes->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        return;
    }
    ui->txtBytes->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString("%1").arg(bswap(val), 1, 10, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtBin_textEdited(const QString & arg1)
{
    bool ok = false;
    QString text = arg1;
    text = text.replace(" ", "").leftJustified(sizeof(duint) * 8, '0', true);
    ULONGLONG val = text.toULongLong(&ok, 2);
    if(!ok)
    {
        ui->txtBin->setStyleSheet("border: 2px solid red;background-color: rgb(49, 49, 49);");
        return;
    }
    ui->txtBin->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString("%1").arg(val, 1, 10, QChar('0')).toUpper());
}

void CalculatorDialog::on_txtAscii_textEdited(const QString & arg1)
{
    QString text = arg1;
    ui->txtAscii->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString().asprintf("%X", text[0].unicode()));
    ui->txtAscii->setCursorPosition(0);
    ui->txtAscii->selectAll();
}

void CalculatorDialog::on_txtUnicode_textEdited(const QString & arg1)
{
    QString text = arg1;
    ui->txtUnicode->setStyleSheet("background-color: rgb(49, 49, 49);");
    ui->txtExpression->setText(QString().asprintf("%X", text[0].unicode()));
    ui->txtUnicode->setCursorPosition(0);
    ui->txtUnicode->selectAll();
}
