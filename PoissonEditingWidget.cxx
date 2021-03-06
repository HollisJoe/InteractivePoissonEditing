/*=========================================================================
 *
 *  Copyright David Doria 2011 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "PoissonEditingWidget.h"

// Custom
#include "ImageFileSelector.h"

// Submodules
#include "ITKHelpers/ITKHelpers.h"
#include "QtHelpers/QtHelpers.h"
#include "ITKQtHelpers/ITKQtHelpers.h"
#include "Mask/Mask.h"
#include "Mask/MaskQt.h"
#include "PoissonEditing/PoissonEditingWrappers.h"

// ITK
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

// Qt
#include <QIcon>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QtConcurrentRun>

PoissonEditingWidget::PoissonEditingWidget()
{
  this->setupUi(this);

  // Instantiate a progress dialog and set it up in marquee mode
  this->ProgressDialog = new QProgressDialog();
  this->ProgressDialog->setMinimum(0);
  this->ProgressDialog->setMaximum(0);
  this->ProgressDialog->setWindowModality(Qt::WindowModal);

  connect(&this->FutureWatcher, SIGNAL(finished()), this, SLOT(slot_IterationComplete()));
  connect(&this->FutureWatcher, SIGNAL(finished()), this->ProgressDialog , SLOT(cancel()));

  this->Image = ImageType::New();
  this->MaskImage = Mask::New();
  this->Result = ImageType::New();

  this->Scene = new QGraphicsScene;
  this->graphicsView->setScene(this->Scene);
}

PoissonEditingWidget::PoissonEditingWidget(const std::string& imageFileName,
                                           const std::string& maskFileName) : PoissonEditingWidget()
{
  std::cout << "PoissonEditingWidget(string, string)" << std::endl;

  this->SourceImageFileName = imageFileName;
  this->MaskImageFileName = maskFileName;
  OpenImageAndMask(this->SourceImageFileName, this->MaskImageFileName);
}

void PoissonEditingWidget::showEvent ( QShowEvent * )
{
  if(this->ImagePixmapItem)
  {
    this->graphicsView->fitInView(this->ImagePixmapItem, Qt::KeepAspectRatio);
  }
}

void PoissonEditingWidget::resizeEvent ( QResizeEvent * )
{
  if(this->ImagePixmapItem)
  {
    this->graphicsView->fitInView(this->ImagePixmapItem, Qt::KeepAspectRatio);
  }
}

void PoissonEditingWidget::on_btnFill_clicked()
{
  typedef PoissonEditing<float> PoissonEditingType;

  typedef PoissonEditingType::GuidanceFieldType GuidanceFieldType;

  GuidanceFieldType::Pointer zeroGuidanceField =
      PoissonEditing<float>::CreateZeroGuidanceField(this->Image.GetPointer());

  std::vector<GuidanceFieldType::Pointer> guidanceFields(3, zeroGuidanceField);

  // We must get a function pointer to the overload that would be chosen by the compiler
  // to pass to run().
  void (*functionPointer)(const std::remove_pointer<decltype(this->Image.GetPointer())>::type*,
                          const std::remove_pointer<decltype(this->MaskImage.GetPointer())>::type*,
                          const std::remove_pointer<decltype(zeroGuidanceField.GetPointer())>::type*,
                          decltype(this->Result.GetPointer()),
                          decltype(this->Image->GetLargestPossibleRegion()),
                          const std::remove_pointer<decltype(this->Image.GetPointer())>::type*)
          = FillImage;

  auto functionToCall =
      std::bind(functionPointer,
                        this->Image.GetPointer(),
                        this->MaskImage.GetPointer(),
                        zeroGuidanceField,
                        this->Result.GetPointer(),
                        this->Image->GetLargestPossibleRegion(), nullptr);

  QFuture<void> future =
        QtConcurrent::run(functionToCall);

  this->FutureWatcher.setFuture(future);

  this->ProgressDialog->exec();
}

void PoissonEditingWidget::on_actionSaveResult_triggered()
{
  // Get a filename to save
  QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".",
                                                  "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  if(fileName.toStdString().empty())
  {
    std::cout << "Filename was empty." << std::endl;
    return;
  }

  ITKHelpers::WriteImage(this->Result.GetPointer(), fileName.toStdString());
  ITKHelpers::WriteRGBImage(this->Result.GetPointer(), fileName.toStdString() + ".png");
  this->statusBar()->showMessage("Saved result.");
}

void PoissonEditingWidget::OpenImageAndMask(const std::string& imageFileName,
                                            const std::string& maskFileName)
{
  // Load and display image
  typedef itk::ImageFileReader<ImageType> ImageReaderType;
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imageFileName);
  imageReader->Update();

  ITKHelpers::DeepCopy(imageReader->GetOutput(), this->Image.GetPointer());

  QImage qimageImage = ITKQtHelpers::GetQImageColor(this->Image.GetPointer(),
                                                    QImage::Format_RGB888);
  this->ImagePixmapItem = this->Scene->addPixmap(QPixmap::fromImage(qimageImage));
  this->graphicsView->fitInView(this->ImagePixmapItem);
  this->ImagePixmapItem->setVisible(this->chkShowInput->isChecked());

  // Load and display mask
  this->MaskImage->Read(maskFileName);

  QImage qimageMask = MaskQt::GetQtImage(this->MaskImage, 122);
  QPixmap maskPixmap = QPixmap::fromImage(qimageMask);

  this->MaskImagePixmapItem = this->Scene->addPixmap(maskPixmap);
  this->MaskImagePixmapItem->setVisible(this->chkShowMask->isChecked());
}

void PoissonEditingWidget::on_actionOpenImageAndMask_triggered()
{
  std::cout << "on_actionOpenImage_triggered" << std::endl;
  std::vector<std::string> namedImages;
  namedImages.push_back("Image");
  namedImages.push_back("Mask");
  
  std::vector<std::string> extensionFilters;
  extensionFilters.push_back("png");
  extensionFilters.push_back("mask");

  ImageFileSelector* fileSelector(new ImageFileSelector(namedImages, extensionFilters));
  fileSelector->exec();

  int result = fileSelector->result();
  if(result) // The user clicked 'ok'
  {
    OpenImageAndMask(fileSelector->GetNamedImageFileName("Image"),
                     fileSelector->GetNamedImageFileName("Mask"));
  }
  else
  {
    // std::cout << "User clicked cancel." << std::endl;
    // The user clicked 'cancel' or closed the dialog, do nothing.
  }
}

void PoissonEditingWidget::on_chkShowInput_clicked()
{
  if(!this->ImagePixmapItem)
  {
    return;
  }
  this->ImagePixmapItem->setVisible(this->chkShowInput->isChecked());
}

void PoissonEditingWidget::on_chkShowOutput_clicked()
{
  if(!this->ResultPixmapItem)
  {
    return;
  }
  this->ResultPixmapItem->setVisible(this->chkShowOutput->isChecked());
}

void PoissonEditingWidget::on_chkShowMask_clicked()
{
  if(!this->MaskImagePixmapItem)
  {
    return;
  }
  this->MaskImagePixmapItem->setVisible(this->chkShowMask->isChecked());
}

void PoissonEditingWidget::slot_IterationComplete()
{
  QImage qimage = ITKQtHelpers::GetQImageColor(this->Result.GetPointer(),
                                               QImage::Format_RGB888);
  this->ResultPixmapItem = this->Scene->addPixmap(QPixmap::fromImage(qimage));
  this->ResultPixmapItem->setVisible(this->chkShowOutput->isChecked());
}
