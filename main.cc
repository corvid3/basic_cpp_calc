#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct Range {
    unsigned start, end;
};

struct CompileContext {
    std::string src;

    std::string get_from_range(Range const range) const noexcept {
        return src.substr(range.start, range.end - range.start);
    }
};

class Token {
   public:
    enum class Type {
        Number,
        Plus,
        Minus,
        Asterisk,
        Solidus,
        LeftParanthesis,
        RightParanthesis,
        Equals,

        Identifier,
    };

    std::string debug_print() const noexcept {
        std::string out;

        switch (m_type) {
            case Type::Number:
                out += "Number";
                break;

            case Type::Identifier:
                out += "Ident";

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

            case Type::Equals:
                out += "Equals";
                break;

            default:
                out += "unimplemented";
        };

        return out;
    }

    Type m_type;

    // a range into the source
    Range m_range;
};

std::vector<Token> tokenize(std::string const& input) {
    std::vector<Token> toks{};

    for (unsigned idx = 0; idx < input.length(); idx++) {
        switch (input[idx]) {
            case ' ':
                while (input[idx + 1] == ' ')
                    idx += 1;
                break;

            case '+':
                toks.push_back(
                    Token{.m_type = Token::Type::Plus,
                          .m_range = {.start = idx, .end = idx + 1}});
                break;

            case '-':
                toks.push_back(
                    Token{.m_type = Token::Type::Minus,
                          .m_range = {.start = idx, .end = idx + 1}});
                break;

            case '*':
                toks.push_back(
                    Token{.m_type = Token::Type::Asterisk,
                          .m_range = {.start = idx, .end = idx + 1}});
                break;

            case '/':
                toks.push_back(
                    Token{.m_type = Token::Type::Solidus,
                          .m_range = {.start = idx, .end = idx + 1}});
                break;

            case '(':
                toks.push_back(
                    Token{.m_type = Token::Type::LeftParanthesis,
                          .m_range = {.start = idx, .end = idx + 1}});
                break;

            case ')':
                toks.push_back(
                    Token{.m_type = Token::Type::RightParanthesis,
                          .m_range = {.start = idx, .end = idx + 1}});
                break;

            default: {
                unsigned start = idx;

                if (isdigit(input[idx]) or input[idx] == '.') {
                    while (isdigit(input[idx]) or input[idx] == '.')
                        idx += 1;

                    toks.push_back(
                        Token{.m_type = Token::Type::Number,
                              .m_range = {.start = start, .end = idx}});
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
    CompileContext const& m_ctx;

    std::vector<Token> const& m_toks;
    int m_idx;

    Parser(CompileContext const& ctx, std::vector<Token> const& toks)
        : m_ctx(ctx), m_toks(toks), m_idx(0) {}

    std::unique_ptr<Node> parse_fact() {
        auto const& tok = m_toks[m_idx];
        m_idx += 1;

        switch (tok.m_type) {
            case Token::Type::Number: {
                std::string str = m_ctx.get_from_range(tok.m_range);
                try {
                    return std::make_unique<NumberNode>(
                        NumberNode(std::stod(str)));
                } catch (std::exception const& e) {
                    throw std::runtime_error("invalid number conversion error");
                }
            };

            case Token::Type::Plus:
            case Token::Type::Minus:
            case Token::Type::Asterisk:
            case Token::Type::Solidus:
            case Token::Type::RightParanthesis:
            case Token::Type::Equals:
            case Token::Type::Identifier:
                throw std::runtime_error("Invalid token in parse stream\n");

            case Token::Type::LeftParanthesis:
                auto ret_val = parse_expr();
                if (m_toks[m_idx].m_type != Token::Type::RightParanthesis)
                    throw std::runtime_error("Expected a left-paranthesis\n");
                return ret_val;
        }
    }

    std::unique_ptr<Node> parse_term() {
        auto left = parse_fact();

        for (;;) {
            auto const& op = m_toks[m_idx];

            if (op.m_type != Token::Type::Asterisk and
                op.m_type != Token::Type::Solidus)
                break;

            m_idx += 1;

            auto right = parse_term();

            left = std::make_unique<BinaryNode>(
                BinaryNode(op.m_type == Token::Type::Asterisk
                               ? BinaryNode::Action::Multiply
                               : BinaryNode::Action::Divide,
                           std::move(left), std::move(right)));
        }

        return left;
    }

    std::unique_ptr<Node> parse_expr() {
        auto left = parse_term();

        for (;;) {
            auto const& op = m_toks[m_idx];

            if (op.m_type != Token::Type::Plus and
                op.m_type != Token::Type::Minus)
                break;

            m_idx += 1;

            auto right = parse_fact();

            left = std::make_unique<BinaryNode>(BinaryNode(
                op.m_type == Token::Type::Plus ? BinaryNode::Action::Add
                                               : BinaryNode::Action::Subtract,
                std::move(left), std::move(right)));
        }

        return left;
    }

   public:
    static std::unique_ptr<Node> parse(CompileContext const& ctx,
                                       std::vector<Token> const& toks) {
        return Parser(ctx, toks).parse_expr();
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
            CompileContext ctx = {
                .src = input,
            };

            auto const toks = tokenize(input);
            auto parse = Parser::parse(ctx, toks);

            std::vector<double> stack;
            parse->execute(stack);

            if (stack.size() > 0)
                std::cout << std::to_string(stack.back()) << std::endl;
        } catch (std::exception const& e) {
            std::cout << e.what() << std::endl;
        }
    }
}
