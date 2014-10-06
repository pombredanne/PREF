#include "disassemblerview.h"
#include "ui_disassemblerview.h"

DisassemblerView::DisassemblerView(ProcessorLoader *loader, QHexEditData *hexeditdata, const QString &viewname, QLabel *labelinfo, QWidget *parent): AbstractView(hexeditdata, viewname, labelinfo, parent), ui(new Ui::DisassemblerView), _listing(nullptr), _loader(loader)
{
    ui->setupUi(this);
    ui->hSplitter->setStretchFactor(0, 1);
    ui->vSplitter->setStretchFactor(0, 1);
    ui->dvSplitter->setStretchFactor(1, 1);

    ui->hexEdit->setReadOnly(true);
    ui->hexEdit->setData(hexeditdata);

    this->_toolbar = new QToolBar();
    this->_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    this->_toolbar->setEnabled(false);

    ui->verticalLayout->insertWidget(0, this->_toolbar);

    this->_actentrypoints = this->_toolbar->addAction(QIcon(":/action_icons/res/entry.png"), "Entry Points");
    this->_actsegments = this->_toolbar->addAction(QIcon(":/action_icons/res/segments.png"), "Segments");
    this->_toolbar->addSeparator();
    this->_actgoto = this->_toolbar->addAction(QIcon(":/action_icons/res/goto.png"), "Goto");

    this->_stringrefs = new StringOffsetModel(this->_hexeditdata, ui->tvStrings);
    ui->tvStrings->setModel(this->_stringrefs);

    connect(this->_actentrypoints, SIGNAL(triggered()), this, SLOT(showEntryPoints()));
    connect(this->_actsegments, SIGNAL(triggered()), this, SLOT(showSegments()));

    this->createListingMenu();
    this->createFunctionsMenu();
}

DisassemblerView::~DisassemblerView()
{
    delete ui;
}

QHexEditData *DisassemblerView::data()
{
    return this->_hexeditdata;
}

bool DisassemblerView::canSave() const
{
    return false;
}

void DisassemblerView::updateStatusBar()
{
    this->updateInfoText(QString());
}

void DisassemblerView::log(const QString &text)
{
    ui->logWidget->write(text);
}

void DisassemblerView::logLine(const QString &text, LogWidget::LogLevel loglevel)
{
    ui->logWidget->writeLine(text, loglevel);
}

void DisassemblerView::createListingMenu()
{
    this->_listingmenu = new QMenu();

    this->_actcrossreferences = this->_listingmenu->addAction(QIcon(":/misc_icons/res/crossreference.png"), "Cross Refernces");
    this->_acthexdump = this->_listingmenu->addAction(QIcon(":/misc_icons/res/hex.png"), "Hex Dump");
    this->_listingmenu->addSeparator();

    QMenu* copymenu = this->_listingmenu->addMenu(QIcon(":/action_icons/res/copy.png"), "Copy");
    this->_actcopyaddress = copymenu->addAction("Address");
    this->_actcopylisting = copymenu->addAction("Listing");

    connect(this->_actcrossreferences, SIGNAL(triggered()), this, SLOT(onListingMenuCrossReferencesTriggered()));
    connect(this->_acthexdump, SIGNAL(triggered()), this, SLOT(onListingMenuHexDumpTriggered()));
}

void DisassemblerView::createFunctionsMenu()
{
    this->_functionsmenu = new QMenu();
    QAction* actjump = this->_functionsmenu->addAction(QIcon(":/action_icons/res/goto.png"), "Jump To Address");
    QAction* actxrefs = this->_functionsmenu->addAction(QIcon(":/misc_icons/res/crossreference.png"), "Cross References");

    connect(actjump, SIGNAL(triggered()), this, SLOT(gotoFunction()));
    connect(actxrefs, SIGNAL(triggered()), this, SLOT(onFunctionsMenuXRefsTriggered()));
}

void DisassemblerView::disassemble()
{
    if(!this->_hexeditdata)
        return;

    this->_listing = new DisassemblerListing(this->_hexeditdata, this);
    this->_loader->callMap(this->_listing, this->_hexeditdata, ui->logWidget);
    this->_loader->disassemble(this->_hexeditdata);
    this->displayDisassembly();
}

void DisassemblerView::on_disassemblerWidget_customContextMenuRequested(const QPoint &pos)
{
    this->_listingmenu->exec(ui->disassemblerWidget->mapToGlobal(pos));
}

void DisassemblerView::on_functionList_customContextMenuRequested(const QPoint &pos)
{
    this->_functionsmenu->exec(ui->functionList->mapToGlobal(pos));
}

void DisassemblerView::onFunctionsMenuXRefsTriggered()
{
    QItemSelectionModel* model = ui->functionList->selectionModel();
    QModelIndex index = model->currentIndex();

    if(index.isValid())
    {
        CrossReferenceDialog crd(reinterpret_cast<Function*>(index.internalPointer()), this->_listing);
        int res = crd.exec();

        if(res == CrossReferenceDialog::Accepted && crd.selectedBlock())
            ui->disassemblerWidget->jumpTo(crd.selectedBlock());
    }
}

void DisassemblerView::onListingMenuCrossReferencesTriggered()
{
    ReferenceTable* referencetable = this->_listing->referenceTable();
    Block* b = ui->disassemblerWidget->selectedBlock();

    if(!referencetable->isReferenced(b))
        return;

    ReferenceSet* refset = nullptr;

    if(b->blockType() == Block::ReferenceBlock)
        refset = qobject_cast<ReferenceSet*>(b);
    else
        refset = referencetable->references(b);

    QList<Reference*> references = refset->referenceList();
    CrossReferenceDialog crd(refset, references, this->_listing);
    int res = crd.exec();

    if(res == CrossReferenceDialog::Accepted && crd.selectedBlock())
        ui->disassemblerWidget->jumpTo(crd.selectedBlock());
}

void DisassemblerView::onListingMenuHexDumpTriggered()
{    
    Block* b = ui->disassemblerWidget->selectedBlock();

    switch(b->blockType())
    {
        case Block::SegmentBlock:
        {
            Segment* s = qobject_cast<Segment*>(b);
            ui->hexEdit->setSelectionRange(s->baseOffset().compatibleValue<qint64>(), s->sizeValue().compatibleValue<qint64>());
            break;
        }

        case Block::FunctionBlock:
        {
            Segment* s = this->_listing->findSegment(b);
            Function* f = qobject_cast<Function*>(b);
            ui->hexEdit->setSelectionRange(((f->startAddress() - s->startAddress()) + s->baseOffset()).compatibleValue<qint64>(), f->sizeValue().compatibleValue<qint64>());
            break;
        }

        case Block::InstructionBlock:
        {
            Instruction* i = qobject_cast<Instruction*>(b);
            ui->hexEdit->setSelectionRange(i->offsetValue().compatibleValue<qint64>(), i->sizeValue().compatibleValue<qint64>());
            break;
        }

        default:
            return;
    }

    ui->tabWidget->setCurrentIndex(1);
}

void DisassemblerView::displayDisassembly()
{
    this->_toolbar->setEnabled(true);
    this->_functionmodel = new FunctionModel(this->_listing, ui->functionList);
    ui->disassemblerWidget->setListing(this->_listing);

    ui->functionList->setModel(this->_functionmodel);
    ui->functionList->resizeRowsToContents();

    for(int i = 0; i < this->_functionmodel->columnCount(); i++)
        ui->functionList->resizeColumnToContents(i);

    /* String Reference Part */
    //this->_stringrefs->setListing(this->_listing);

    /* DataMap Page */
    //ui->dataMapView->setListing(this->_listing);
}

void DisassemblerView::showEntryPoints()
{
    EntryPointsDialog epd(this->_listing, this);
    epd.exec();
}

void DisassemblerView::showSegments()
{
    SegmentsDialog sd(this->_listing, this);
    sd.exec();
}

void DisassemblerView::gotoFunction()
{
    QItemSelectionModel* selectionmodel = ui->functionList->selectionModel();

    if(selectionmodel->hasSelection())
        ui->disassemblerWidget->jumpTo(reinterpret_cast<Function*>(selectionmodel->selectedIndexes()[0].internalPointer()));
}

void DisassemblerView::on_functionList_doubleClicked(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    ui->disassemblerWidget->jumpTo(reinterpret_cast<Function*>(index.internalPointer()));
}
