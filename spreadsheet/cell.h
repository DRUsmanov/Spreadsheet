#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "common.h"

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();
    void Set(std::string text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;       
private:
    void ResetCache();
    bool IsCached() const;   
    void ResetDependentCellsCache();
    bool ChekCircularDependency() const;
    void UpdateDependencies();
    void RemoveDependencies();
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    SheetInterface& sheet_;
    std::optional<std::vector<Cell*>> dependent_cells_;
    std::unique_ptr<Impl> impl_;
    std::string prev_text_; // Для сохранения текста, которым была инициализирована ячейка ранее
};