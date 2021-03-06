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

/** This class takes a collection of strings that are image "names" (simply
  * unique identifiers). For each image name provided, a panel is created to
  * select and preview the image. Once all requested images have been selected,
  * the dialog can be closed and the results are available in the member FileNames.
  */

#ifndef ImageFileSelector_H
#define ImageFileSelector_H

#include "ui_FileSelector.h"

// Submodules
#include "Mask/Mask.h"

// Custom
#include "FileSelectionWidget.h"
#include "Panel.h"

// Qt
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsView>

// ITK
#include "itkVectorImage.h"

typedef itk::VectorImage<float, 2> ImageType;

class ImageFileSelector : public QDialog
//class PoissonEditingFileSelector : public QWidget
{
  Q_OBJECT
public:

  ImageFileSelector(const std::vector<std::string>& namedImages, const std::vector<std::string>& extensionFilters);
  std::string GetNamedImageFileName(const std::string& namedImage);
  
public slots:
  
  void Verify();
  void slot_buttonBox_accepted();
  void slot_buttonBox_rejected();
  
protected:
  std::vector<std::string> FileNames; // The results (the selected file names) are stored in this for later retrieval.
  std::vector<std::string> NamedImages; // The identifiers of the images to load
  std::vector<std::string> ExtensionFilters; // The filter for the file extension for each image

  std::vector<Panel*> Panels; // Can't store non-pointers because Qt doesn't allow it.
  QDialogButtonBox* ButtonBox;
};

#endif // ImageFileSelector_H
