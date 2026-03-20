#include "cell.h"
#include "common.h"
#include "formula.h"


#include <deque>
#include <optional>
#include <set>
#include <string>
#include <algorithm>


class Cell::Impl {
public:
	virtual CellInterface::Value GetValue(const SheetInterface& sheet) const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const = 0;
	virtual bool IsCached() const = 0;
	virtual void ResetCache() = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
	CellInterface::Value GetValue([[maybe_unused]]const SheetInterface& sheet) const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;
	bool IsCached() const override;
	void ResetCache() override;
};

class Cell::TextImpl : public Cell::Impl {
public:
	TextImpl() = default;
	TextImpl(std::string text);
	CellInterface::Value GetValue([[maybe_unused]] const SheetInterface& sheet) const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;
	bool IsCached() const override;
	void ResetCache() override;
private:
	std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
	FormulaImpl() = default;
	FormulaImpl(std::string text);
	CellInterface::Value GetValue(const SheetInterface& sheet) const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;
	bool IsCached() const override;
	void ResetCache() override;
private:
	mutable std::optional<CellInterface::Value> cache_;	
	std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(SheetInterface& sheet)
	:sheet_{sheet}
	,impl_{std::make_unique<Cell::EmptyImpl>()}{
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	if (text == prev_text_) {
		return;
	}
	std::unique_ptr<Impl> new_value;
	if (text.empty()) {
		new_value.reset(new Cell::EmptyImpl());
	}
	else if (text[0] != FORMULA_SIGN || (text.size() == 1 && text[0] == FORMULA_SIGN)) {
		new_value.reset(new Cell::TextImpl(text));
	}
	else if (text[0] == FORMULA_SIGN) {
		new_value.reset(new Cell::FormulaImpl(text.substr(1)));
	}
	impl_.swap(new_value);
	if (!ChekCircularDependency()) {
		impl_.swap(new_value);
		throw CircularDependencyException("Error: circular dependency");
	}
	impl_.swap(new_value);
	ResetCache();
	RemoveDependencies();
	impl_.swap(new_value);
	UpdateDependencies();
	prev_text_ = text;
}

void Cell::Clear() {
	std::unique_ptr<Impl> new_value;
	new_value.reset(new Cell::EmptyImpl());
	ResetCache();
	RemoveDependencies();
	impl_ = std::move(new_value);
	UpdateDependencies();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue(sheet_);
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

bool Cell::IsCached() const {
	return impl_->IsCached();
}

void Cell::ResetDependentCellsCache() {
	if (dependent_cells_.has_value()) {
		for (auto& cell : dependent_cells_.value()) {
			if (cell->IsCached()) {
				cell->ResetCache();
			}			
		}
	}
}

void Cell::ResetCache() {
	impl_->ResetCache();
	ResetDependentCellsCache();
}

bool Cell::ChekCircularDependency() const {
	std::vector<Position> referenced_cells = impl_->GetReferencedCells();
	std::deque<Position> posions_queue{ referenced_cells.begin(), referenced_cells.end() };
	std::set<Position> unique_positions;
	while (!posions_queue.empty()) {
		Position pos = posions_queue.front();
		posions_queue.pop_front();
		auto [iterator, is_inserted] = unique_positions.insert(pos);
		if (!is_inserted) {
			return false;
		}
		const CellInterface* cell = sheet_.GetCell(pos);
		std::vector<Position> includ_references_cells;
		if (cell) {
			includ_references_cells = cell->GetReferencedCells();
		}
		for (const auto& pos : includ_references_cells) {
			posions_queue.push_back(pos);
		}
	}
	return true;
}

void Cell::UpdateDependencies() {
	std::vector<Position> referenced_pos = impl_->GetReferencedCells();
	for (const auto& pos : referenced_pos) {
		CellInterface* cell_interface = sheet_.GetCell(pos);
		if (cell_interface) {
			Cell* cell = dynamic_cast<Cell*>(cell_interface);
			if (dependent_cells_.has_value()) {
				cell->dependent_cells_.value().push_back(this);
			}
			else {
				cell->dependent_cells_ = std::vector<Cell*>{ this };
			}			
		}		
	}
}

void Cell::RemoveDependencies() {
	std::vector<Position> referenced_pos = impl_->GetReferencedCells();
	for (auto& pos : referenced_pos) {
		CellInterface* cell_interface = sheet_.GetCell(pos);
		if (cell_interface) {
			Cell* cell = dynamic_cast<Cell*>(cell_interface);
			if (dependent_cells_.has_value()) {
				std::vector<Cell*> included_dependent_cells = cell->dependent_cells_.value();
				included_dependent_cells.erase(
					std::remove(included_dependent_cells.begin(), included_dependent_cells.end(), this),
					included_dependent_cells.end());
				if (included_dependent_cells.empty()) {
					cell->dependent_cells_.reset();
				}
			}			
		}
	}
}

CellInterface::Value Cell::EmptyImpl::GetValue([[maybe_unused]] const SheetInterface& sheet) const {
	return "";
}

std::string Cell::EmptyImpl::GetText() const {
	return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
	return {};
}

bool Cell::EmptyImpl::IsCached() const {
	return false;
}

void Cell::EmptyImpl::ResetCache() {
	//Ничего не делает
}

Cell::TextImpl::TextImpl(std::string text)
	:text_{text} {
}

CellInterface::Value Cell::TextImpl::GetValue([[maybe_unused]] const SheetInterface& sheet) const {
	if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
		return text_.substr(1);
	}
	return text_;	
}

std::string Cell::TextImpl::GetText() const {
	return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
	return {};
}

bool Cell::TextImpl::IsCached() const {
	return false;
}

void Cell::TextImpl::ResetCache() {
	//Ничего не делает
}

Cell::FormulaImpl::FormulaImpl(std::string text)
	: formula_{ParseFormula(text)} {
}

CellInterface::Value Cell::FormulaImpl::GetValue(const SheetInterface& sheet) const {
	if (IsCached()) {
		return cache_.value();
	}
	FormulaInterface::Value value = formula_->Evaluate(sheet);
	if (std::holds_alternative<double>(value)) {
		double val = std::get<double>(value);
		cache_ = val;
		return val;
	}
	else {
		return std::get<FormulaError>(value);
	}
}

std::string Cell::FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_.get()->GetReferencedCells();
}

bool Cell::FormulaImpl::IsCached() const {
	return cache_.has_value();
}

void Cell::FormulaImpl::ResetCache() {
	cache_.reset();
}

