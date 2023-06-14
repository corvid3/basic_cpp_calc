#include <cctype>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct Token {
    enum class Type {
        Number,
        Plus,
        Minus,
        Asterisk,
        Solidus,
        LeftParanthesis,
        RightParanthesis,
    };

    std::string debug_print() const noexcept {
        std::string out;

        switch (this->type) {
            case Type::Number:
                out += "Number ";
                out += std::to_string(this->opt_val);
                break;
            case Type::Plus:
                out += "Plus";
                break;
            case Type::Minus:
                out += "Minus";
                break;
            case Type::Asterisk:
                out += "Asterisk";
                break;
            case Type::Solidus:
                out += "Solidus";
                break;
            case Type::LeftParanthesis:
                out += "LeftParanthesis";
                break;
            case Type::RightParanthesis:
                out += "RightParanthesis";
                break;
        };

        return out;
    }

    Type type;
    double opt_val;
};

std::vector<Token> tokenize(std::string const& input) {
    std::vector<Token> toks{};

    for (int idx = 0; idx < input.length(); idx++) {
        switch (input[idx]) {
            case ' ':
                while (input[idx + 1] == ' ')
                    idx += 1;
                break;

            case '+':
                toks.push_back(Token{.type = Token::Type::Plus, .opt_val = 0});
                break;

            case '-':
                toks.push_back(Token{.type = Token::Type::Minus, .opt_val = 0});
                break;

            case '*':
                toks.push_back(
                    Token{.type = Token::Type::Asterisk, .opt_val = 0});
                break;

            case '/':
                toks.push_back(
                    Token{.type = Token::Type::Solidus, .opt_val = 0});
                break;

            case '(':
                toks.push_back(
                    Token{.type = Token::Type::LeftParanthesis, .opt_val = 0});
                break;

            case ')':
                toks.push_back(
                    Token{.type = Token::Type::RightParanthesis, .opt_val = 0});
                break;

            default: {
                if (isdigit(input[idx]) or input[idx] == '.') {
                    std::string temp;

                    while (isdigit(input[idx]) or input[idx] == '.')
                        temp += input[idx++];

                    try {
                        double v = std::stod(temp);
                        toks.push_back(
                            Token{.type = Token::Type::Number, .opt_val = v});
                    } catch (std::exception const& e) {
                        throw std::runtime_error(
                            "Failure to parse a number.\n");
                    }

                    // dumb hack
                    idx--;
                } else {
                    // just throw an exception for now
                    throw std::runtime_error(
                        "Letters are not a valid token.\n");
                }
            } break;
        }
    }

    return toks;
}

class Node {
   protected:
    Node() = default;

   public:
    virtual void execute(std::vector<double>& stack) = 0;
    virtual ~Node() = default;
};

class NumberNode : public Node {
    double m_number;

   public:
    NumberNode(double number) : m_number(number) {}

    virtual void execute(std::vector<double>& stack) override {
        stack.push_back(m_number);
    }
};

class BinaryNode : public Node {
   public:
    enum Action {
        Add,
        Subtract,
        Multiply,
        Divide,
    };

    BinaryNode(Action action,
               std::unique_ptr<Node>&& left,
               std::unique_ptr<Node>&& right)
        : m_action(action),
          m_left(std::move(left)),
          m_right(std::move(right)) {}

    void execute(std::vector<double>& stack) override {
        m_left->execute(stack);
        m_right->execute(stack);

        auto right_val = stack.back();
        stack.pop_back();
        auto left_val = stack.back();
        stack.pop_back();

        switch (m_action) {
            case Add:
                stack.push_back(left_val + right_val);
                break;

            case Subtract:
                stack.push_back(left_val - right_val);
                break;

            case Multiply:
                stack.push_back(left_val * right_val);
                break;

            case Divide:
                stack.push_back(left_val / right_val);
                break;
        }
    }

   private:
    Action m_action;
    std::unique_ptr<Node> m_left, m_right;
};

class Parser {
    int m_idx;
    std::vector<Token> const& m_toks;

    Parser(std::vector<Token> const& toks) : m_toks(toks), m_idx(0) {}

    std::unique_ptr<Node> parse_fact() {
        auto const& tok = m_toks[m_idx];
        m_idx += 1;

        switch (tok.type) {
            case Token::Type::Number:
                return std::make_unique<NumberNode>(NumberNode(tok.opt_val));

            case Token::Type::Plus:
            case Token::Type::Minus:
            case Token::Type::Asterisk:
            case Token::Type::Solidus:
            case Token::Type::RightParanthesis:
                throw std::runtime_error("Invalid token in parse stream\n");

            case Token::Type::LeftParanthesis:
                auto ret_val = parse_expr();
                if (m_toks[m_idx].type != Token::Type::RightParanthesis)
                    throw std::runtime_error("Expected a left-paranthesis\n");
                return ret_val;
        }
    }

    std::unique_ptr<Node> parse_term() {
        auto left = parse_fact();

        for (;;) {
            auto const& op = m_toks[m_idx];

            if (op.type != Token::Type::Asterisk and
                op.type != Token::Type::Solidus)
                break;

            m_idx += 1;

            auto right = parse_term();

            left = std::make_unique<BinaryNode>(BinaryNode(
                op.type == Token::Type::Asterisk ? BinaryNode::Action::Multiply
                                                 : BinaryNode::Action::Divide,
                std::move(left), std::move(right)));
        }

        return left;
    }

    std::unique_ptr<Node> parse_expr() {
        auto left = parse_term();

        for (;;) {
            auto const& op = m_toks[m_idx];

            if (op.type != Token::Type::Plus and op.type != Token::Type::Minus)
                break;

            m_idx += 1;

            auto right = parse_fact();

            left = std::make_unique<BinaryNode>(BinaryNode(
                op.type == Token::Type::Plus ? BinaryNode::Action::Add
                                             : BinaryNode::Action::Subtract,
                std::move(left), std::move(right)));
        }

        return left;
    }

   public:
    static std::unique_ptr<Node> parse(std::vector<Token> const& toks) {
        return Parser(toks).parse_expr();
    }
};

int main() {
    std::cout << "Type \"quit\" to leave.\n";

    while (true) {
        std::cout << ">> ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "quit")
            break;

        try {
            auto const toks = tokenize(input);

            auto parse = Parser::parse(toks);

            std::vector<double> stack;
            parse->execute(stack);

            if (stack.size() > 0)
                std::cout << std::to_string(stack.back()) << std::endl;
        } catch (std::exception const& e) {
            std::cout << e.what() << std::endl;
        }
    }
}
