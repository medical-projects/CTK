/*=========================================================================

  Library:   CTK
 
  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 
=========================================================================*/

// Qt includes
#include <QDebug>
#include <QStack>

// CTK includes
#include "ctkModelTester.h"

//-----------------------------------------------------------------------------
class ctkModelTesterPrivate: public ctkPrivate<ctkModelTester>
{
public:
  ctkModelTesterPrivate();
  QAbstractItemModel *Model;
  bool ThrowOnError;
  bool NestedInserts;

  struct Change
  {
    QModelIndex Parent;
    Qt::Orientation Orientation;
    int Start;
    int End;
    
    int Count;
    QList<QPersistentModelIndex> Items;
  };

  QStack<Change> AboutToBeInserted;
  QStack<Change> AboutToBeRemoved;
  QList<QPersistentModelIndex> LayoutAboutToBeChanged;
};

//-----------------------------------------------------------------------------
// ctkModelTesterPrivate methods

//-----------------------------------------------------------------------------
ctkModelTesterPrivate::ctkModelTesterPrivate()
{
  this->Model = 0;
  this->ThrowOnError = true;
  this->NestedInserts = false;
}

//-----------------------------------------------------------------------------
// ctkModelTester methods

//-----------------------------------------------------------------------------
ctkModelTester::ctkModelTester(QObject *_parent)
  :QObject(_parent)
{
  CTK_INIT_PRIVATE(ctkModelTester);
}

//-----------------------------------------------------------------------------
ctkModelTester::ctkModelTester(QAbstractItemModel *_model, QObject *_parent)
  :QObject(_parent)
{
  CTK_INIT_PRIVATE(ctkModelTester);
  this->setModel(_model);
}

//-----------------------------------------------------------------------------
void ctkModelTester::setModel(QAbstractItemModel *_model)
{
  CTK_D(ctkModelTester);
  if (d->Model)
    {
    // disconnect
    d->Model->disconnect(this);
    d->AboutToBeInserted.clear();
    d->AboutToBeRemoved.clear();
    d->LayoutAboutToBeChanged.clear();
    }
  if (_model)
    {
    connect(_model, SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(onColumnsAboutToBeInserted(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(onColumnsAboutToBeRemoved(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
            this, SLOT(onColumnsInserted(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
            this, SLOT(onColumnsRemoved(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(onDataChanged(const QModelIndex& , const QModelIndex &)));
    connect(_model, SIGNAL(layoutAboutToBeChanged()), this, SLOT(onLayoutAboutToBeChanged()));
    connect(_model, SIGNAL(layoutChanged()), this, SLOT(onLayoutChanged()));
    connect(_model, SIGNAL(modelAboutToBeReset()), this, SLOT(onModelAboutToBeReset()));
    connect(_model, SIGNAL(modelReset()), this, SLOT(onModelReset()));
    connect(_model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(onRowsAboutToBeInserted(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(onRowsAboutToBeRemoved(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(onRowsInserted(const QModelIndex& , int, int)));
    connect(_model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this, SLOT(onRowsRemoved(const QModelIndex& , int, int)));
    }
  d->Model = _model;
  this->testModel();
}

//-----------------------------------------------------------------------------
QAbstractItemModel* ctkModelTester::model()const
{
  return ctk_d()->Model;
}

//-----------------------------------------------------------------------------
void ctkModelTester::setThrowOnError(bool throwException)
{
  ctk_d()->ThrowOnError = throwException;
}

//-----------------------------------------------------------------------------
bool ctkModelTester::throwOnError()const
{
  return ctk_d()->ThrowOnError;
}

//-----------------------------------------------------------------------------
void ctkModelTester::setNestedInserts( bool nestedInsertsValue )
{
  ctk_d()->NestedInserts = nestedInsertsValue;
}

//-----------------------------------------------------------------------------
bool ctkModelTester::nestedInserts()const
{
  return ctk_d()->NestedInserts;
}

//-----------------------------------------------------------------------------
void  ctkModelTester::test(bool result, const QString& errorString)const
{
  if (result)
    {
    return;
    }
  qDebug() << errorString;
  if (this->throwOnError())
    {
    throw errorString;
    }
}

//-----------------------------------------------------------------------------
void ctkModelTester::testModelIndex(const QModelIndex& index)const
{
  CTK_D(const ctkModelTester);
  if (!index.isValid())
    {// invalid index
    this->test(index.model() == 0, "An invalid index can't have a valid model.");
    this->test(index.model() != d->Model, "An invalid index can't have a valid model.");
    this->test(index.column() == -1, "An invalid index can't have a valid column.");
    this->test(index.row() == -1, "An invalid index can't have a valid row.");
    this->test(index.parent().isValid() == false, "An invalid index can't have a valid row.");
    this->test(index.row() == -1, "An invalid index can't have a valid row.");
    for(int i = 0; i < 100; ++i)
      {
      this->test(index.sibling(i % 10, i / 10).isValid() == false, "An invalid index can't have valid sibling.");
      }
    }
  else
    {// valid index
    this->test(index.model() == d->Model, "A valid index must have a valid model.");
    this->test(index.column() >= 0, "An valid index can't have an invalid column.");
    this->test(index.row() >= 0, "An valid index can't have an invalid row.");
    this->test(index == index.sibling(index.row(), index.column()), "Index's row and/or column is wrong.");
    }
  this->testData(index);
  this->testParent(index);
}

//-----------------------------------------------------------------------------
void ctkModelTester::testData(const QModelIndex& index)const
{
  if (!index.isValid())
    {
    this->test(!index.data(Qt::DisplayRole).isValid(), 
               QString("An invalid index can't have valid data: %1")
               .arg(index.data(Qt::DisplayRole).toString()));
    }
  else
    {
    this->test(index.data(Qt::DisplayRole).isValid(), 
               QString("A valid index can't have invalid data: %1, %2, %3")
               .arg(index.row()).arg(index.column()).arg(long(index.internalPointer())));
    }
}

//-----------------------------------------------------------------------------
void ctkModelTester::testParent(const QModelIndex& vparent)const
{
  CTK_D(const ctkModelTester);
  if (!d->Model->hasChildren(vparent))
    {
    // it's asking a lot :-)
    //this->test(d->Model->columnCount(vparent) <= 0, "A parent with no children can't have a columnCount > 0.");
    this->test(d->Model->rowCount(vparent) <= 0, "A parent with no children can't have a rowCount > 0.");
    }
  else
    {
    this->test(d->Model->columnCount(vparent) > 0, "A parent with children can't have a columnCount <= 0.");
    this->test(d->Model->rowCount(vparent) > 0 || d->Model->canFetchMore(vparent), "A parent with children can't have a rowCount <= 0. or if it does, canFetchMore should return true");
    }

  if (!vparent.isValid())
    {// otherwise there will be an infinite loop
    return;
    }
  
  for (int i = 0 ; i < d->Model->rowCount(vparent); ++i)
    {
    for (int j = 0; j < d->Model->columnCount(vparent); ++j)
      {
      this->test(d->Model->hasIndex(i, j, vparent), "hasIndex should return true for int range {0->rowCount(), 0->columnCount()}");
      QModelIndex child = vparent.child(i, j);
      this->test(child.parent() == vparent, "A child's parent can't be different from its parent");
      this->testModelIndex(child);
      }
    }
}
//-----------------------------------------------------------------------------
void ctkModelTester::testPersistentModelIndex(const QPersistentModelIndex& index)const
{
  CTK_D(const ctkModelTester);
  //qDebug() << "Test persistent Index: " << index ;
  this->test(index.isValid(), "Persistent model index can't be invalid");
  // did you forget to call QAbstractItemModel::changePersistentIndex() between 
  // beginLayoutChanged() and changePersistentIndex() ?
  QModelIndex modelIndex = d->Model->index(index.row(), index.column(), index.parent());
  this->test(modelIndex == index, 
             QString("Persistent index (%1, %2) can't be invalid").arg(index.row()).arg(index.column()));
}

//-----------------------------------------------------------------------------
void ctkModelTester::testModel()const
{
  CTK_D(const ctkModelTester);
  if (d->Model == 0)
    {
    return;
    }
  for (int i = 0 ; i < d->Model->rowCount(); ++i)
    {
    for (int j = 0; j < d->Model->columnCount(); ++j)
      {
      this->test(d->Model->hasIndex(i, j), "hasIndex should return true for int range {0->rowCount(), 0->columnCount()}");
      QModelIndex child = d->Model->index(i, j);
      this->test(!child.parent().isValid(), "A child's parent can't be different from its parent");
      this->testModelIndex(child);
      }
    }
}

//-----------------------------------------------------------------------------
void ctkModelTester::onColumnsAboutToBeInserted(const QModelIndex & vparent, int start, int end)
{
  //qDebug() << "columnsAboutToBeInserted: " << vparent << start << end;
  this->onItemsAboutToBeInserted(vparent, Qt::Horizontal, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onColumnsAboutToBeRemoved(const QModelIndex & vparent, int start, int end)
{
  //qDebug() << "columnsAboutToBeRemoved: " << vparent << start << end;
  this->onItemsAboutToBeRemoved(vparent, Qt::Horizontal, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onColumnsInserted(const QModelIndex & vparent, int start, int end)
{
  //qDebug() << "columnsInserted: " << vparent << start << end;
  this->onItemsInserted(vparent, Qt::Horizontal, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onColumnsRemoved(const QModelIndex & vparent, int start, int end)
{
  //qDebug() << "columnsRemoved: " << vparent << start << end;
  this->onItemsRemoved(vparent, Qt::Horizontal, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
  this->test(topLeft.parent() == bottomRight.parent(), "DataChanged support items with the same parent only");
  this->test(topLeft.row() >= bottomRight.row(), "topLeft can't have a row lower than bottomRight");
  this->test(bottomRight.column() >= topLeft.column(), "topLeft can't have a column lower than bottomRight");
  for (int i = topLeft.row(); i <= bottomRight.row(); ++i)
    {
    for (int j = topLeft.column(); j < bottomRight.column(); ++j)
      {
      this->test(topLeft.sibling(i,j).isValid(), "Changed data must be valid");
      // do the test on the indexes here, it's easier to debug than in testModel();
      this->testModelIndex(topLeft.sibling(i,j));
      }
    }
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
  CTK_D(ctkModelTester);
  this->test(first <= last, "Changed headers have wrong indexes");
  switch (orientation)
    {
    case Qt::Horizontal:
      this->test(d->Model->columnCount() > last, "There is no more horizontal headers than columns.");
      break;
    case Qt::Vertical:
      this->test(d->Model->rowCount() > last, "There is no more vertical headers than rows.");
      break;
    default:
      this->test(orientation == Qt::Horizontal || orientation == Qt::Vertical, "Wrong orientation.");
      break;
    }
  this->testModel();
}

//-----------------------------------------------------------------------------
QList<QPersistentModelIndex> ctkModelTester::persistentModelIndexes(const QModelIndex& index)const
{
  CTK_D(const ctkModelTester);
  QList<QPersistentModelIndex> list;
  for (int i = 0; i < d->Model->rowCount(index); ++i)
    {
    for (int j = 0; j < d->Model->columnCount(index); ++j)
      {
      QPersistentModelIndex child = d->Model->index(i, j, index);
      list.append(child);
      list += this->ctkModelTester::persistentModelIndexes(child);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void ctkModelTester::onLayoutAboutToBeChanged()
{
  CTK_D(ctkModelTester);

  d->LayoutAboutToBeChanged = this->persistentModelIndexes(QModelIndex());
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onLayoutChanged()
{
  CTK_D(ctkModelTester);
  foreach (const QPersistentModelIndex& index, d->LayoutAboutToBeChanged)
    {
    this->testPersistentModelIndex(index);
    }
  d->LayoutAboutToBeChanged.clear();
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onModelAboutToBeReset()
{
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onModelReset()
{
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onRowsAboutToBeInserted(const QModelIndex &vparent, int start, int end)
{
  //qDebug() << "rowsAboutToBeInserted: " << vparent << start << end;
  this->onItemsAboutToBeInserted(vparent, Qt::Vertical, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onRowsAboutToBeRemoved(const QModelIndex &vparent, int start, int end)
{
  //qDebug() << "rowsAboutToBeRemoved: " << vparent << start << end;
  this->onItemsAboutToBeRemoved(vparent, Qt::Vertical, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onRowsInserted(const QModelIndex & vparent, int start, int end)
{
  //qDebug() << "rowsInserted: " << vparent << start << end;
  this->onItemsInserted(vparent, Qt::Vertical, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onRowsRemoved(const QModelIndex & vparent, int start, int end)
{
  //qDebug() << "rowsRemoved: " << vparent << start << end;
  this->onItemsRemoved(vparent, Qt::Vertical, start, end);
}

//-----------------------------------------------------------------------------
void ctkModelTester::onItemsAboutToBeInserted(const QModelIndex &vparent, Qt::Orientation orientation, int start, int end)
{
  CTK_D(ctkModelTester);
  this->test(start <= end, "Start can't be higher than end");
  //Not sure about that
  if (!d->NestedInserts)
    {
    this->test(d->AboutToBeInserted.size() == 0, "While inserting items, you can't insert other items.");
    }
  //Not sure about that
  this->test(d->AboutToBeRemoved.size() == 0, "While removing items, you can't insert other items.");

  ctkModelTesterPrivate::Change change;
  change.Parent = vparent;
  change.Orientation = orientation;
  change.Start = start;
  change.End = end;
  change.Count = (orientation == Qt::Vertical ? d->Model->rowCount(vparent) :d->Model->columnCount(vparent) );
  change.Items = this->persistentModelIndexes(vparent);
  d->AboutToBeInserted.push(change);
  
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onItemsAboutToBeRemoved(const QModelIndex &vparent, Qt::Orientation orientation, int start, int end)
{
  CTK_D(ctkModelTester);
  this->test(start <= end, "Start can't be higher than end");
  //Not sure about that
  this->test(d->AboutToBeInserted.size() == 0, "While inserting items, you can't remove other items.");
  //Not sure about that
  this->test(d->AboutToBeRemoved.size() == 0, "While removing items, you can't remove other items.");
  
  int count = (orientation == Qt::Vertical ? d->Model->rowCount(vparent) :d->Model->columnCount(vparent) );
  this->test(start < count, "Item to remove can't be invalid");
  this->test(end < count, "Item to remove can't be invalid");
  
  ctkModelTesterPrivate::Change change;
  change.Parent = vparent;
  change.Orientation = orientation;
  change.Start = start;
  change.End = end;
  change.Count = count;
  for (int i = 0 ; i < count; ++i)
    {
    QPersistentModelIndex index;
    index = (orientation == Qt::Vertical ? d->Model->index(i, 0, vparent) : d->Model->index(0, i, vparent));
    this->test(index.isValid(), "Index invalid");
    if (orientation == Qt::Vertical && (index.row() < start || index.row() > end))
      {
      change.Items.append(index);
      }
    if (orientation == Qt::Horizontal && (index.column() < start || index.column() > end))
      {
      change.Items.append(index);
      }
    }
  d->AboutToBeRemoved.push(change);

  this->testModel();
  //qDebug() << "About to be removed: " << start << " " << end <<vparent << count << change.Items.count();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onItemsInserted(const QModelIndex & vparent, Qt::Orientation orientation, int start, int end)
{
  CTK_D(ctkModelTester);
  this->test(start <= end, "Start can't be higher end");
  this->test(d->AboutToBeInserted.size() != 0, "rowsInserted() has been emitted, but not rowsAboutToBeInserted.");
  //Not sure about that
  this->test(d->AboutToBeRemoved.size() == 0, "While removing items, you can't insert other items.");

  ctkModelTesterPrivate::Change change = d->AboutToBeInserted.pop();
  this->test(change.Parent == vparent, "Parent can't be different");
  this->test(change.Orientation == Qt::Vertical, "Orientation can't be different");
  this->test(change.Start == start, "Start can't be different");
  this->test(change.End == end, "End can't be different");
  int count =  (orientation == Qt::Vertical ? d->Model->rowCount(vparent) :d->Model->columnCount(vparent) );
  this->test(change.Count < count, "The new count number can't be lower");
  this->test(count - change.Count == (end - start + 1) , "The new count number can't be lower");
  foreach(const QPersistentModelIndex& index, change.Items)
    {
    this->testPersistentModelIndex(index);
    }
  change.Items.clear();
  
  this->testModel();
}

//-----------------------------------------------------------------------------
void ctkModelTester::onItemsRemoved(const QModelIndex & vparent, Qt::Orientation orientation, int start, int end)
{ 
  CTK_D(ctkModelTester);
  this->test(start <= end, "Start can't be higher end");
  this->test(d->AboutToBeRemoved.size() != 0, "rowsRemoved() has been emitted, but not rowsAboutToBeRemoved.");
  //Not sure about that
  this->test(d->AboutToBeInserted.size() == 0, "While inserted items, you can't remove other items.");

  ctkModelTesterPrivate::Change change = d->AboutToBeRemoved.pop();
  this->test(change.Parent == vparent, "Parent can't be different");
  this->test(change.Orientation == Qt::Vertical, "Orientation can't be different");
  this->test(change.Start == start, "Start can't be different");
  this->test(change.End == end, "End can't be different");
  int count = (orientation == Qt::Vertical ? d->Model->rowCount(vparent) :d->Model->columnCount(vparent) );
  this->test(change.Count > count, "The new count number can't be higher");
  this->test(change.Count - count == (end - start + 1) , "The new count number can't be higher");
  foreach(const QPersistentModelIndex& index, change.Items)
    {
    this->testPersistentModelIndex(index);
    }
  change.Items.clear();
  
  this->testModel();
}

