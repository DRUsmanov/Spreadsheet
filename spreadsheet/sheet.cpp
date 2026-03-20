#include "cell.h"
#include "common.h"
#include "sheet.h"

#include <iostream>


using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    if (sheet_.find(pos) == sheet_.end()) {
        sheet_.emplace(pos, std::make_unique<Cell>(*this));
    }
    try {        
        sheet_[pos]->Set(text);        
    }
    catch (const FormulaException& exp) {
        sheet_.erase(pos);
        throw exp;
    }
    const Cell* cell = GetCell(pos);
    std::vector<Position> referenced_cell = cell->GetReferencedCells();
    //Создает пустые ячейки, которые были применены в формуле, но ранее не были заданы методом Set
    for (const auto& pos : referenced_cell) {
        if (!GetCell(pos)) {
            sheet_.emplace(pos, std::make_unique<Cell>(*this));
        }
    }
}

const Cell* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

Cell* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    if (sheet_.find(pos) == sheet_.end()) {
        return nullptr;
    }
    return sheet_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    if (sheet_.find(pos) == sheet_.end()) {
        return;
    }
    if (sheet_[pos]) {
        sheet_[pos].get()->Clear();
    }    
    sheet_.erase(pos);
}

Size Sheet::GetPrintableSize() const {
    Size size;
    for (const auto& [key, value] : sheet_) {
        size.rows = key.row + 1 > size.rows ? key.row + 1 : size.rows;
        size.cols = key.col + 1 > size.cols ? key.col + 1 : size.cols;
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int r = 0; r < size.rows; ++r) {
        bool is_first_col = true;
        for (int c = 0; c < size.cols; ++c) {
            if (!is_first_col) {
                output << '\t';
            }
            const CellInterface* cell = GetCell({ r,c });
            if (cell) {
                CellInterface::Value value = cell->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                }
                else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                }
                else if (std::holds_alternative<FormulaError>(value)) {
                    output << std::get<FormulaError>(value);
                }
            }
            else {
                output << "";
            }            
            is_first_col = false;
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int r = 0; r < size.rows; ++r) {
        bool is_first_col = true;
        for (int c = 0; c < size.cols; ++c) {
            if (!is_first_col) {
                output << '\t';
            }
            const CellInterface* cell = GetCell({ r,c });
            output << (cell ? cell->GetText() : "");
            is_first_col = false;
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}