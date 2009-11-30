/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "ColorType.h"
#include "CustomColorButton.h"
#include "DataVariantEditor.h"
#include "DateTime.h"
#include "DynamicObject.h"
#include "Filename.h"
#include "ObjectFactory.h"
#include "ObjectResource.h"
#include "StringUtilities.h"
#include "SymbolTypeGrid.h"

#include <QtCore/QDateTime>
#include <QtGui/QDateTimeEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QStackedWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QValidator>

#include <string>
#include <vector>
using namespace std;

vector<DataVariantEditorDelegate> DataVariantEditor::sDelegates;

DataVariantEditor::DataVariantEditor(QWidget* pParent, bool includeValueLabel) :
   QWidget(pParent)
{
   // Value
   QLabel* pValueLabel = includeValueLabel ? new QLabel("Value:", this) : NULL;

   mpStack = new QStackedWidget(this);

   QWidget* pValueWidget = new QWidget(mpStack);
   mpValueLineEdit = new QLineEdit(pValueWidget);

   QVBoxLayout* pValueLayout = new QVBoxLayout(pValueWidget);
   pValueLayout->setMargin(0);
   pValueLayout->setSpacing(5);
   pValueLayout->addWidget(mpValueLineEdit);
   pValueLayout->addStretch();

   mpValueList = new QListWidget(mpStack);
   mpValueTextEdit = new QTextEdit(mpStack);

   QWidget* pBoolWidget = new QWidget(mpStack);
   QGroupBox* pBoolGroup = new QGroupBox(pBoolWidget);
   mpFalseRadio = new QRadioButton("False", pBoolGroup);
   mpTrueRadio = new QRadioButton("True", pBoolGroup);

   QVBoxLayout* pBoolGroupGrid = new QVBoxLayout(pBoolGroup);
   pBoolGroupGrid->setMargin(10);
   pBoolGroupGrid->setSpacing(5);
   pBoolGroupGrid->addWidget(mpFalseRadio);
   pBoolGroupGrid->addWidget(mpTrueRadio);

   QVBoxLayout* pBoolGrid = new QVBoxLayout(pBoolWidget);
   pBoolGrid->setMargin(0);
   pBoolGrid->setSpacing(10);
   pBoolGrid->addWidget(pBoolGroup, 0, Qt::AlignLeft);
   pBoolGrid->addStretch();

   QWidget* pDateTimeWidget = new QWidget(mpStack);
   mpDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), pDateTimeWidget);

   QVBoxLayout* pDateTimeLayout = new QVBoxLayout(pDateTimeWidget);
   pDateTimeLayout->setMargin(0);
   pDateTimeLayout->setSpacing(5);
   pDateTimeLayout->addWidget(mpDateTimeEdit);
   pDateTimeLayout->addStretch(10);

   mpIntValidator = new QIntValidator(NULL);
   mpDoubleValidator = new QDoubleValidator(NULL);

   QWidget* pColorWidget = new QWidget(mpStack);
   mpColorEdit = new CustomColorButton(pColorWidget);
   QVBoxLayout* pColorGrid = new QVBoxLayout(pColorWidget);
   pColorGrid->setMargin(0);
   pColorGrid->setSpacing(10);
   pColorGrid->addWidget(mpColorEdit, 0, Qt::AlignLeft);
   pColorGrid->addStretch();

   QWidget* pSymbolTypeWidget = new QWidget(mpStack);
   mpSymbolTypeEdit = new SymbolTypeGrid(pSymbolTypeWidget);
   mpSymbolTypeEdit->setBorderedSymbols(true);
   QVBoxLayout* pSymbolTypeGrid = new QVBoxLayout(pSymbolTypeWidget);
   pSymbolTypeGrid->setMargin(0);
   pSymbolTypeGrid->setSpacing(10);
   pSymbolTypeGrid->addWidget(mpSymbolTypeEdit, 0, Qt::AlignLeft);
   pSymbolTypeGrid->addStretch();

   mpStack->addWidget(pValueWidget);
   mpStack->addWidget(mpValueList);
   mpStack->addWidget(mpValueTextEdit);
   mpStack->addWidget(pBoolWidget);
   mpStack->addWidget(pDateTimeWidget);
   mpStack->addWidget(pColorWidget);
   mpStack->addWidget(pSymbolTypeWidget);

   // Browse button
   QIcon icnBrowse(":/icons/Open");

   mpBrowseButton = new QPushButton(icnBrowse, QString(), this);
   mpBrowseButton->setFixedWidth(27);
   mpBrowseButton->hide();

   // Layout
   QGridLayout* pGrid = new QGridLayout(this);
   pGrid->setMargin(0);
   pGrid->setSpacing(5);
   int row = 0;
   if (includeValueLabel)
   {
      pGrid->addWidget(pValueLabel, row++, 0, 1, 2);
   }
   pGrid->addWidget(mpStack, row, 0);
   pGrid->addWidget(mpBrowseButton, row, 1, Qt::AlignTop);
   pGrid->setRowStretch(row, 10);
   pGrid->setColumnStretch(0, 10);

   // Initialization
   setStackWidget("");

   // Connections
   VERIFYNR(connect(mpBrowseButton, SIGNAL(clicked()), this, SLOT(browse())));
   VERIFYNR(connect(mpValueLineEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(modified())));
   VERIFYNR(connect(mpValueList, SIGNAL(itemSelectionChanged()), this, SIGNAL(modified())));
   VERIFYNR(connect(mpValueTextEdit, SIGNAL(textChanged()), this, SIGNAL(modified())));
   VERIFYNR(connect(mpFalseRadio, SIGNAL(toggled(bool)), this, SIGNAL(modified())));
   VERIFYNR(connect(mpTrueRadio, SIGNAL(toggled(bool)), this, SIGNAL(modified())));
   VERIFYNR(connect(mpDateTimeEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SIGNAL(modified())));
   VERIFYNR(connect(mpColorEdit, SIGNAL(colorChanged(const QColor&)), this, SIGNAL(modified())));
   VERIFYNR(connect(mpSymbolTypeEdit, SIGNAL(valueChanged(SymbolType)), this, SIGNAL(modified())));
}

DataVariantEditor::~DataVariantEditor()
{
}

const vector<DataVariantEditorDelegate>& DataVariantEditor::getDelegates()
{
   if (sDelegates.empty() == true)
   {
      sDelegates.push_back(DataVariantEditorDelegate("char", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("signed char", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("unsigned char", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("short", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("unsigned short", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("int", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("unsigned int", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("long", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("unsigned long", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("int64_t", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("Int64", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("UInt64", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("uint64_t", DataVariantEditorDelegate::INTEGRAL));
      sDelegates.push_back(DataVariantEditorDelegate("float", DataVariantEditorDelegate::DOUBLE));
      sDelegates.push_back(DataVariantEditorDelegate("double", DataVariantEditorDelegate::DOUBLE));
      sDelegates.push_back(DataVariantEditorDelegate("bool", DataVariantEditorDelegate::BOOL));
      sDelegates.push_back(DataVariantEditorDelegate("string", DataVariantEditorDelegate::TEXT));
      sDelegates.push_back(DataVariantEditorDelegate("Filename", DataVariantEditorDelegate::TEXT, true));
      sDelegates.push_back(DataVariantEditorDelegate("ColorType", DataVariantEditorDelegate::COLOR));
      sDelegates.push_back(DataVariantEditorDelegate("DateTime", DataVariantEditorDelegate::DATE_TIME));
      DataVariantEditorDelegate temp = DataVariantEditorDelegate("DisplayMode", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<DisplayMode>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("EncodingType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<EncodingType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("EndianType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<EndianType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("GeocoordType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<GeocoordType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("InterleaveFormatType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<InterleaveFormatType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("LayerType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<LayerType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("UnitType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<UnitType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("PassArea", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<PassArea>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("ProcessingLocation", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<ProcessingLocation>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("RasterChannelType", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<RasterChannelType>());
      sDelegates.push_back(temp);
      temp = DataVariantEditorDelegate("RegionUnits", DataVariantEditorDelegate::ENUMERATION);
      temp.setEnumValueStrings(StringUtilities::getAllEnumValuesAsDisplayString<RegionUnits>());
      sDelegates.push_back(temp);
      sDelegates.push_back(DataVariantEditorDelegate("SymbolType", DataVariantEditorDelegate::SYMBOL_TYPE));
      sDelegates.push_back(DataVariantEditorDelegate("vector<char>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<signed char>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<unsigned char>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<short>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<unsigned short>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<int>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<unsigned int>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<long>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<unsigned long>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<int64_t>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<uint64_t>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<Int64>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<UInt64>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<float>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<double>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<bool>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<string>", DataVariantEditorDelegate::VECTOR));
      sDelegates.push_back(DataVariantEditorDelegate("vector<Filename>", DataVariantEditorDelegate::VECTOR, true));
      sDelegates.push_back(DataVariantEditorDelegate("DynamicObject", DataVariantEditorDelegate::DYNAMIC_OBJECT));
   }

   return sDelegates;
}

DataVariantEditorDelegate DataVariantEditor::getDelegate(const string& type)
{
   DataVariantEditorDelegate retVal;

   const vector<DataVariantEditorDelegate>& delegates = getDelegates();
   for (vector<DataVariantEditorDelegate>::const_iterator iter = delegates.begin(); iter != delegates.end(); ++iter)
   {
      if (iter->getTypeName() == type)
      {
         retVal = *iter;
         break;
      }
   }
   return retVal;
}

bool DataVariantEditor::hasDelegate(const string& type)
{
   const vector<DataVariantEditorDelegate>& delegates = getDelegates();
   for (vector<DataVariantEditorDelegate>::const_iterator iter = delegates.begin(); iter != delegates.end(); ++iter)
   {
      if (iter->getTypeName() == type)
      {
         return true;
      }
   }
   return false;
}

QSize DataVariantEditor::sizeHint() const
{
   return mpStack->currentWidget()->sizeHint();
}

void DataVariantEditor::setValue(const DataVariant& value, bool useVariantCurrentValue)
{
#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : The useVariantCurrentValue parameter is used to not " \
   "initialize a widget with a default value only when it would otherwise initialize to an undefined value.  The " \
   "parameter can be removed when DataVariant supports a valid type with an invalid value. (dsulgrov)")

   if (value.isValid() == false)
   {
      return;
   }

   // Update the widgets
   string type = value.getTypeName();

   DataVariantEditorDelegate curDelegate = getDelegate(type);
   if (curDelegate.getType() == DataVariantEditorDelegate::BOOL)
   {
      bool boolValue = dv_cast<bool>(value, true);
      if (boolValue)
      {
         mpTrueRadio->setChecked(true);
      }
      else
      {
         mpFalseRadio->setChecked(true);
      }
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::DATE_TIME)
   {
      QDateTime dateTime;
      if (useVariantCurrentValue)
      {
         const DateTime* pDateTime = dv_cast<DateTime>(&value);
         if (pDateTime != NULL)
         {
            time_t timeValue = pDateTime->getStructured();
            dateTime = QDateTime::fromTime_t(timeValue);
         }
      }

      mpDateTimeEdit->setDateTime(dateTime);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::ENUMERATION)
   {
      // Populate the list widget with the enumerated values
      mpValueList->clear();
      string valueText = value.toDisplayString();

      const vector<string>& enumValues = curDelegate.getEnumValueStrings();
      for (vector<string>::size_type i = 0; i < enumValues.size(); ++i)
      {
         string enumValue = enumValues[i];
         if (enumValue.empty() == false)
         {
            mpValueList->addItem(QString::fromStdString(enumValue));
            if (enumValue == valueText)
            {
               mpValueList->setCurrentRow(static_cast<int>(i));
            }
         }
      }

      if ((mpValueList->currentRow() == -1) && (mpValueList->count() > 0))
      {
         mpValueList->setCurrentRow(0);
      }
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::VECTOR)
   {
      mpValueTextEdit->clear();
      if (useVariantCurrentValue)
      {
         if (curDelegate.getTypeName() == "vector<string>")
         {
            vector<string> values = dv_cast<vector<string> >(value);
            for (vector<string>::const_iterator iter = values.begin(); iter != values.end(); ++iter)
            {
               QString valueText = QString::fromStdString(*iter);
               mpValueTextEdit->append(valueText);
            }
         }
         else if (curDelegate.getTypeName() == "vector<Filename>")
         {
            vector<Filename*> values = dv_cast<vector<Filename*> >(value);
            for (vector<Filename*>::const_iterator iter = values.begin(); iter != values.end(); ++iter)
            {
               Filename* pFilename = *iter;
               if (pFilename != NULL)
               {
                  QString valueText = QString::fromStdString(pFilename->getFullPathAndName());
                  mpValueTextEdit->append(valueText);
               }
            }
         }
         else
         {
            QString valueText = QString::fromStdString(value.toDisplayString());

            QStringList strlValues = valueText.split(", ");
            for (int i = 0; i < strlValues.count(); ++i)
            {
               mpValueTextEdit->append(strlValues[i]);
            }
         }
      }
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::COLOR)
   {
      ColorType color = dv_cast<ColorType>(value, ColorType());
      mpColorEdit->setColor(color);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::SYMBOL_TYPE)
   {
      SymbolType symbol = dv_cast<SymbolType>(value, SymbolType());
      mpSymbolTypeEdit->setCurrentValue(symbol);
   }
   else
   {
      mpValueLineEdit->clear();
      if (useVariantCurrentValue)
      {
         QString valueText = QString::fromStdString(value.toDisplayString());
         mpValueLineEdit->setText(valueText);
      }
   }

   setStackWidget(type);
   mValue = value;
}

const DataVariant& DataVariantEditor::getValue()
{
   DataVariantEditorDelegate curDelegate = getDelegate(mValue.getTypeName());
   if (curDelegate.getType() == DataVariantEditorDelegate::BOOL)
   {
      bool boolValue = false;
      if (mpTrueRadio->isChecked() == true)
      {
         boolValue = true;
      }

      mValue = boolValue;
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::DATE_TIME)
   {
      QDateTime dateTimeValue = mpDateTimeEdit->dateTime();
      time_t timeValue = dateTimeValue.toTime_t();

      FactoryResource<DateTime> pDateTime;
      pDateTime->setStructured(timeValue);

      mValue = *pDateTime.get();
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::ENUMERATION)
   {
      QString valueText;

      QListWidgetItem* pItem = mpValueList->currentItem();
      if (pItem != NULL)
      {
         valueText = pItem->text();
      }

      mValue.fromDisplayString(mValue.getTypeName(), valueText.toStdString());
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::DYNAMIC_OBJECT)
   {
      FactoryResource<DynamicObject> pObj;
      mValue = *pObj.get();
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::VECTOR)
   {
      QString valueText = mpValueTextEdit->toPlainText();
      if (curDelegate.getTypeName() == "vector<string>")
      {
         vector<string> values;

         QStringList items = valueText.split("\n");
         for (int i = 0; i < items.count(); ++i)
         {
            QString itemText = items[i];
            values.push_back(itemText.toStdString());
         }

         mValue = values;
      }
      else if (curDelegate.getTypeName() == "vector<Filename>")
      {
         vector<Filename*> values;

         QStringList items = valueText.split("\n");
         for (int i = 0; i < items.count(); ++i)
         {
            FactoryResource<Filename> pFilename;
            pFilename->setFullPathAndName(items[i].toStdString());
            values.push_back(pFilename.release());
         }

         mValue = values;

         Service<ObjectFactory> pFactory;
         for (vector<Filename*>::iterator iter = values.begin(); iter != values.end(); ++iter)
         {
            Filename* pFilename = *iter;
            if (pFilename != NULL)
            {
               pFactory->destroyObject(pFilename, "Filename");
            }
         }
      }
      else
      {
         valueText.replace("\n", ", ");
         mValue.fromDisplayString(mValue.getTypeName(), valueText.toStdString());
      }
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::COLOR)
   {
      mValue = mpColorEdit->getColorType();
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::SYMBOL_TYPE)
   {
      mValue = mpSymbolTypeEdit->getCurrentValue();
   }
   else
   {
      string valueText = mpValueLineEdit->text().toStdString();
      if (mValue.fromDisplayString(mValue.getTypeName(), valueText) == DataVariant::FAILURE)
      {
#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : This should error, fix this when we can have a " \
   "DataVariant with a type and an invalid value (tclarke)")
         // if possible, put a reasonable default in the variant so we don't get garbage
         // types not in this list either have a reasonable value already or we can't determine one at this point
         switch (curDelegate.getType())
         {
         case DataVariantEditorDelegate::INTEGRAL:
         case DataVariantEditorDelegate::DOUBLE:
            mValue.fromDisplayString(mValue.getTypeName(), "0");
            break;
         default:
            // do nothing
            break;
         }
      }
   }

   return mValue;
}

void DataVariantEditor::setStackWidget(string type)
{
   mpValueLineEdit->setEnabled(true);
   mpBrowseButton->hide();

   DataVariantEditorDelegate curDelegate = getDelegate(type);
   if (curDelegate.getTypeName().empty())
   {
      return;
   }

   if (curDelegate.getNeedBrowseButton() == true)
   {
      mpBrowseButton->show();
   }

   if (curDelegate.getType() == DataVariantEditorDelegate::BOOL)
   {
      mpStack->setCurrentIndex(3);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::DATE_TIME)
   {
      mpStack->setCurrentIndex(4);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::INTEGRAL)
   {
#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : Consider using one or more custom QValidator subclasses "\
   "and removing mpIntValidator when implementing OPTICKS-588 (dadkins)")
      mpValueLineEdit->setValidator(NULL);
      mpStack->setCurrentIndex(0);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::DOUBLE)
   {
      mpValueLineEdit->setValidator(mpDoubleValidator);
      mpStack->setCurrentIndex(0);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::TEXT)
   {
      mpValueLineEdit->setValidator(NULL);
      mpStack->setCurrentIndex(0);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::ENUMERATION)
   {
      mpStack->setCurrentIndex(1);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::DYNAMIC_OBJECT)
   {
      mpStack->setCurrentIndex(0);
      mpValueLineEdit->setEnabled(false);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::VECTOR)
   {
      mpStack->setCurrentIndex(2);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::COLOR)
   {
      mpStack->setCurrentIndex(5);
   }
   else if (curDelegate.getType() == DataVariantEditorDelegate::SYMBOL_TYPE)
   {
      mpStack->setCurrentIndex(6);
   }
   else
   {
      VERIFYNRV_MSG(false, "Unrecognized editor type value");
   }
}

void DataVariantEditor::browse()
{
   QWidget* pVisible = mpStack->currentWidget();
   if (pVisible == mpValueTextEdit)
   {
      QStringList strlFilenames = QFileDialog::getOpenFileNames(this, "Get Filenames", QString(), "All Files (*.*)");
      if (strlFilenames.isEmpty() == true)
      {
         return;
      }

      mpValueTextEdit->clear();

      int iCount = 0;
      iCount = strlFilenames.count();
      for (int i = 0; i < iCount; i++)
      {
         QString strFilename = strlFilenames[i];
         if (strFilename.isEmpty() == false)
         {
            mpValueTextEdit->append(strFilename);
         }
      }
   }
   else
   {
      QString strFilename = QFileDialog::getOpenFileName(this, "Get Filename", QString(), "All Files (*.*)");
      if (strFilename.isEmpty() == false)
      {
         mpValueLineEdit->setText(strFilename);
      }
   }
}

#pragma message(__FILE__ "(" STRING(__LINE__) ") : warning : Use a QListView to allow editing of vector data, " \
   "since the fromDisplayString parser is brittle? (kstreith)")
