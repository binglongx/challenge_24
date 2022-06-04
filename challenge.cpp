#include <iostream>
#include <memory>
#include <variant>
#include <vector>
#include <optional>

struct Nothing{};
struct BinaryOperation;

struct Expression
{
    Expression()
    {}  // empty, nothing, anything

    Expression(int x)
    : expression(x)
    {}

    Expression(BinaryOperation&& op)
    : expression(std::make_unique<BinaryOperation>(std::move(op)))
    {}

    Expression(Expression&& expr)
    : expression(std::move(expr.expression))
    {}

    std::variant<Nothing, int, std::unique_ptr<BinaryOperation>> expression;
    
    void print(std::ostream& os) const;
    int evaluate() const;

};

struct BinaryOperation
{
    BinaryOperation(Expression&& left_, char op, Expression&& right_)
    : left(std::make_unique<Expression>(std::move(left_)))
    , operation(op)
    , right(std::make_unique<Expression>(std::move(right_)))
    {}
    
    std::unique_ptr<Expression> left;
    char operation;
    std::unique_ptr<Expression> right;
    
    void print(std::ostream& os) const;
    int evaluate() const;
};

int BinaryOperation::evaluate() const
{
    assert(left && right);
    int left_ = left->evaluate();
    int right_ = right->evaluate();
    switch(operation)
    {
        case '+': return left_ + right_;
        case '-': return left_ - right_;
        case '*': return left_ * right_;
        case '/': return left_ / right_;
        default : assert(false); return 0;
    }
}

void BinaryOperation::print(std::ostream& os) const
{
    os << "( ";
    left->print(os);

    os << " ";
    os << operation;
    os << " ";

    right->print(os);
    os << " )";

}

void Expression::print(std::ostream& os) const
{
    switch(expression.index())
    {
        case 0: os << "[Anything]"; return;
        case 1: os << std::get<1>(expression); return;
        case 2: std::get<2>(expression)->print(os); return;
        default: assert(false); return;
    }
}

int Expression::evaluate() const
{
    switch(expression.index())
    {
        case 0: return 0; // nothing
        case 1: return std::get<1>(expression); // int
        case 2: return std::get<2>(expression)->evaluate(); // binary op.
        default: assert(false); return 0;
    }
}

std::optional<Expression> challenge(const std::vector<int>& numbers, int target);

// given `candidate`, search in `others` to meet `target`
static std::optional<Expression> find_binary_operation(Expression&& candidate, const std::vector<int>& others, int target)
{
    int candidate_value = candidate.evaluate();
    
    // '+'
    if( auto expr = challenge(others, target - candidate_value); expr )
        return BinaryOperation{std::move(candidate), '+', std::move(*expr)};
    // '-'
    if( auto expr = challenge(others, candidate_value - target); expr )
        return BinaryOperation{std::move(candidate), '-', std::move(*expr)};
    // '-'
    if( auto expr = challenge(others, candidate_value + target); expr )
        return BinaryOperation{std::move(*expr), '-', std::move(candidate)};

    // '*' or '/'
    if( target == 0 )
    {
        if( candidate_value == 0 )
        {
            return BinaryOperation{std::move(candidate), '*', {}}; // right could be anything
        }
        else
        {
            if( auto expr = challenge(others, 0); expr )
                return BinaryOperation{std::move(candidate), '*', std::move(*expr)};
        }
    }
    else // target != 0
    {
        if( candidate_value != 0 )
        {
            // '*'
            if( target % candidate_value == 0)
            {
                if( auto expr = challenge(others, target / candidate_value); expr )
                    return BinaryOperation{std::move(candidate), '*', std::move(*expr)};
            }
            if( candidate_value % target == 0 )
            {
                if( auto expr = challenge(others, candidate_value / target); expr )
                    return BinaryOperation{std::move(candidate), '/', std::move(*expr)};
            }
            if( auto expr = challenge(others, candidate_value * target); expr )
                return BinaryOperation{std::move(*expr), '/', std::move(candidate)};
        }
    }
    return {}; // not found
}


std::optional<Expression> challenge(const std::vector<int>& numbers, int target)
{
    if( numbers.size()==1 )
    {
        if( target == numbers[0] )
            return Expression{numbers[0]};
        return {};  // fail
    }

    // single operand initiated: find the complementary.
    for(size_t i=0; i<numbers.size(); ++i)
    {
        int operand = numbers[i];
        std::vector<int> others = numbers;
        others.erase(others.begin() + ptrdiff_t(i));

        if( auto expr = find_binary_operation(Expression{operand}, others, target); expr )
            return expr;
    }

    // binary operation initiated: : find the complementary.
    if( numbers.size()>=4 )
    {
        for(size_t i=0; i<numbers.size()-1; ++i)
        {
            for(size_t j=i+1; j<numbers.size(); ++j)
            {
                int left = numbers[i];
                int right = numbers[j];
                std::vector<int> others = numbers;
                others.erase(others.begin() + ptrdiff_t(j));
                others.erase(others.begin() + ptrdiff_t(i));
                
                // +
                if( auto expr = find_binary_operation(BinaryOperation{left, '+', right}, others, target); expr )
                    return expr;
                // -
                if( auto expr = find_binary_operation(BinaryOperation{left, '-', right}, others, target); expr )
                    return expr;
                if( auto expr = find_binary_operation(BinaryOperation{right, '-', left}, others, target); expr )
                    return expr;
                // *
                if( auto expr = find_binary_operation(BinaryOperation{left, '*', right}, others, target); expr )
                    return expr;
                // '/'
                if( right != 0 && (left % right)==0 )
                {
                    if( auto expr = find_binary_operation(BinaryOperation{left, '/', right}, others, target); expr )
                        return expr;
                }
                if( left != 0 && (right % left)==0 )
                {
                    if( auto expr = find_binary_operation(BinaryOperation{right, '/', left}, others, target); expr )
                        return expr;
                }
            }
        }
    }

    return {}; // fail
}

static void challenge_print(const std::vector<int>& numbers, int target)
{
    std::cout << "Target: " << target;
    std::cout << ",  Numbers: ";
    for( auto num : numbers)
        std::cout << num << " ";
    std::cout << std::endl;
    
    std::cout << "Solving..." << std::flush;
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    auto result = challenge(numbers, target);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-t0).count();
    std::cout << us << " us" << std::endl;
    if( result )
    {
        std::cout << "Solved: ";
        result->print(std::cout);
        std::cout << " = " << result->evaluate() << std::endl;
    }
    else
        std::cout << "No solution found" << std::endl;
}

[[maybe_unused]] static void challenge_print_24(const std::vector<int> &numbers)
{
    return challenge_print(numbers, 24);
}
 
int main()
{
    challenge_print_24(std::vector<int>{1, 3, 1, 5});
    //challenge_print(std::vector<int>{0, 2}, 0);
    return 0;
}
