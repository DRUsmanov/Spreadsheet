#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <sstream>


using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression);
        Value Evaluate(const SheetInterface& sheet) const override;
        std::vector<Position> GetReferencedCells() const override;
        std::string GetExpression() const override;

    private:
        FormulaAST ast_;
    };

    Formula::Formula(std::string expression) try
        : ast_{ParseFormulaAST(expression)} {

    }catch (...) {
        throw FormulaException("Error");
    }

    Formula::Value Formula::Evaluate(const SheetInterface& sheet) const {
        try {
            return ast_.Execute(sheet);
        }
        catch (const FormulaError& exp) {
            return exp;
        }
    }

    std::vector<Position> Formula::GetReferencedCells() const {
        std::vector<Position> result;
        const std::forward_list<Position>& cells = ast_.GetCells();
        for (const auto& cell : cells) {
            result.push_back(cell);
        }
        std::sort(result.begin(), result.end());
        auto pos = std::unique(result.begin(), result.end());
        result.erase(pos, result.end());
        return result;
    }

    std::string Formula::GetExpression() const {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}