#include <Doxybook/Doxygen.hpp>
#include <Doxybook/TextMarkdownPrinter.hpp>
#include <Doxybook/Utils.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>

#include <Doxybook/Log.hpp>

std::string Doxybook2::TextMarkdownPrinter::print(const XmlTextParser::Node& node) const {
    PrintData data;
    print(data, nullptr, &node, nullptr, nullptr);
    auto str = data.ss.str();
    while (!str.empty() && str.back() == '\n')
        str.pop_back();
    return str;
}

namespace {
    std::string formatQtLinks(const std::string& string)
    {
        using namespace std;
        std::string replaced = regex_replace(string, regex(" *\\\\saqt *"), "");
        vector<std::string> strings;
        istringstream functionStream(replaced);
        std::string function;
        std::string ret = "<br><b>See also in the Qt documentation:</b> ";
        while(getline(functionStream, function, ',')) {
            function = regex_replace(function, regex(" "), "");
            istringstream symbolStream(function);
            std::string symbol;
            vector<std::string> symbols;
            while(getline(symbolStream, symbol, ':')) {
                symbols.push_back(symbol);
            }
            ret += "<a href=\"https://doc.qt.io/qt-5/"
                    + Doxybook2::Utils::toLower(symbols.at(0))
                    + ".html";
            if(symbols.size() > 2) {
                std::string anchor = symbols.at(2);
                anchor = regex_replace(anchor, regex("[\\(\\)]"), "");
                ret += "#" + anchor;
                if(anchor == symbols.at(2)) // no parentheses
                {
                    if(anchor.size() > 0 && std::islower(anchor[0])) {
                        ret += "-prop";
                    }
                    // Otherwise it is an enum value, but those don't
                    // have an anchor to link to
                }
            }
            ret += "\" style=\"color: #17a81a\" target=\"_blank\">"
                    + function + " &#x2197;</a>, ";
        }
        ret = regex_replace(ret, regex(", $"), "");
        return ret;
    }
}

void Doxybook2::TextMarkdownPrinter::print(PrintData& data,
    const XmlTextParser::Node* parent,
    const XmlTextParser::Node* node,
    const XmlTextParser::Node* previous,
    const XmlTextParser::Node* next) const {

    switch (node->type) {
        case XmlTextParser::Node::Type::TEXT: {
            if(node->data.find(" \\saqt", 0) == 0) {
                std::string links = formatQtLinks(node->data);
                data.ss << links;
            } else {
                if (config.linkAndInlineCodeAsHTML && data.inComputerOutput) {
                    data.ss << Utils::escape(node->data);
                } else {
                    data.ss << node->data;
                }
            }
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SECT1: {
            data.ss << "\n# ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SECT2: {
            data.ss << "\n## ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SECT3: {
            data.ss << "\n### ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SECT4: {
            data.ss << "\n#### ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SECT5: {
            data.ss << "\n##### ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SECT6: {
            data.ss << "\n###### ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::BOLD: {
            data.ss << "**";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::EMPHASIS: {
            data.ss << "_";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::STRIKE: {
            data.ss << "~~";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::VARIABLELIST:
        case XmlTextParser::Node::Type::ORDEREDLIST:
        case XmlTextParser::Node::Type::ITEMIZEDLIST: {
            if (data.lists.empty())
                data.ss << "\n";
            data.ss << "\n";
            data.eol = true;
            data.lists.push_back({0, node->type == XmlTextParser::Node::Type::ORDEREDLIST});
            break;
        }
        case XmlTextParser::Node::Type::LISTITEM: {
            if (data.lists.size() > 1) {
                data.ss << std::string((data.lists.size() - 1) * 4, ' ');
            }
            data.lists.back().counter++;
            if (data.lists.back().ordered && data.lists.size() == 1) {
                data.ss << data.lists.back().counter << ". ";
            } else {
                data.ss << "* ";
            }
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::ULINK: {
            if (config.linkAndInlineCodeAsHTML) {
                if (!node->extra.empty()) {
                    data.ss << "<a href=\"" << node->extra << "\">";
                    data.validLink = true;
                }
            } else {
                data.ss << "[";
            }
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::REF: {
            if (config.linkAndInlineCodeAsHTML) {
                const auto found = doxygen.getCache().find(node->extra);
                if (found != doxygen.getCache().end()) {
                    data.ss << "<a href=\"" << found->second->getUrl() << "\">";
                    data.validLink = true;
                }
            } else {
                data.ss << "[";
            }
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::IMAGE: {
            const auto prefix = config.baseUrl + config.imagesFolder;
            data.ss << "![" << node->extra << "](" << prefix << (prefix.empty() ? "" : "/") << node->extra << ")";
            data.eol = false;
            if (config.copyImages) {
                std::ifstream src(Utils::join(inputDir, node->extra), std::ios::binary);
                if (src && config.useFolders && !config.imagesFolder.empty()) {
                    std::ofstream dst(
                        Utils::join(config.outputDir, config.imagesFolder, node->extra), std::ios::binary);
                    if (dst)
                        dst << src.rdbuf();
                } else if (src) {
                    std::ofstream dst(Utils::join(config.outputDir, node->extra), std::ios::binary);
                    if (dst)
                        dst << src.rdbuf();
                }
            }
            break;
        }
        case XmlTextParser::Node::Type::COMPUTEROUTPUT: {
            if (config.linkAndInlineCodeAsHTML) {
                data.ss << "<code>";
            } else {
                data.ss << "`";
            }
            data.inComputerOutput = true;
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::PROGRAMLISTING: {
            data.ss << "\n\n```cpp\n";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::VERBATIM: {
            data.ss << "\n\n";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::SP: {
            data.ss << " ";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::HRULER: {
            data.ss << "\n\n------------------\n\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::VARLISTENTRY: {
            data.ss << "\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::SUPERSCRIPT: {
            data.ss << "<sup>";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::NONBREAKSPACE: {
            data.ss << "&nbsp;";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::TABLE: {
            data.ss << "\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::TABLE_ROW: {
            data.ss << "|";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::TABLE_CELL: {
            data.ss << " ";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::SQUO: {
            data.ss << "\"";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::NDASH: {
            data.ss << "&ndash;";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::MDASH: {
            data.ss << "&mdash;";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::LINEBREAK: {
            data.ss << "\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::ONLYFOR: {
            data.ss << "(";
            data.eol = false;
            break;
        }
        default: {
            break;
        }
    }

    switch (node->type) {
        case XmlTextParser::Node::Type::PROGRAMLISTING: {
            programlisting(data.ss, *node);
            break;
        }
        case XmlTextParser::Node::Type::FORMULA: {
            if (node->children.empty()) {
                break;
            }
            const auto& child = node->children.front();
            const auto& formula = child.data;
            if (formula.empty()) {
                break;
            }
            if (formula[0] == '$' && formula.size() >= 3) {
                data.ss << config.formulaInlineStart;
                data.ss << formula.substr(1, formula.size() - 2);
                data.ss << config.formulaInlineEnd;
            } else if (formula.find("\\[") == 0 && formula.size() >= 5) {
                data.ss << config.formulaBlockStart;
                data.ss << formula.substr(2, formula.size() - 4);
                data.ss << config.formulaBlockEnd;
            }
            break;
        }
        default: {
            for (size_t i = 0; i < node->children.size(); i++) {
                const auto childNext = i + 1 < node->children.size() ? &node->children[i + 1] : nullptr;
                const auto childPrevious = i > 0 ? &node->children[i - 1] : nullptr;
                print(data, node, &node->children[i], childPrevious, childNext);
            }
        }
    }

    switch (node->type) {
        case XmlTextParser::Node::Type::TITLE: {
            data.ss << "\n\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::BOLD: {
            data.ss << "**";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::EMPHASIS: {
            data.ss << "_";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::STRIKE: {
            data.ss << "~~";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::ULINK: {
            if (config.linkAndInlineCodeAsHTML) {
                if (data.validLink) {
                    data.ss << "</a>";
                }
            } else {
                data.ss << "]";
                if (!node->extra.empty()) {
                    data.ss << "(" << node->extra << ")";
                }
            }
            data.validLink = false;
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::REF: {
            if (config.linkAndInlineCodeAsHTML) {
                if (data.validLink) {
                    data.ss << "</a>";
                }
            } else {
                data.ss << "]";
                const auto found = doxygen.getCache().find(node->extra);
                if (found != doxygen.getCache().end()) {
                    data.ss << "(" << found->second->getUrl() << ")";
                }
            }
            data.validLink = false;
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::PARA: {
            if (parent && parent->type == XmlTextParser::Node::Type::TABLE_CELL) {
                break;
            }
            if (parent && parent->type == XmlTextParser::Node::Type::LISTITEM) {
                if (!data.eol) {
                    data.ss << "\n";
                    data.eol = true;
                }
            } else {
                if (!data.eol) {
                    data.ss << "\n\n";
                    data.eol = true;
                }
            }
            break;
        }
        case XmlTextParser::Node::Type::COMPUTEROUTPUT: {
            if (config.linkAndInlineCodeAsHTML) {
                data.ss << "</code>";
            } else {
                data.ss << "`";
            }
            data.inComputerOutput = false;
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::ITEMIZEDLIST:
        case XmlTextParser::Node::Type::VARIABLELIST:
        case XmlTextParser::Node::Type::ORDEREDLIST: {
            data.lists.pop_back();
            break;
        }
        case XmlTextParser::Node::Type::LISTITEM: {
            break;
        }
        case XmlTextParser::Node::Type::CODELINE: {
            data.ss << "\n";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::PROGRAMLISTING: {
            data.ss << "```\n\n";
            if (!node->extra.empty()) {
                data.ss << "_Filename: " << node->extra << "_\n\n";
            }
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::VERBATIM: {
            data.ss << "\n\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::VARLISTENTRY: {
            data.ss << "\n\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::SUPERSCRIPT: {
            data.ss << "</sup>";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::TABLE: {
            data.ss << "\n\n";
            data.eol = true;
            data.tableHeader = false;
            break;
        }
        case XmlTextParser::Node::Type::TABLE_ROW: {
            if (!data.tableHeader) {
                data.ss << "\n| ";
                for (size_t i = 0; i < node->children.size(); i++) {
                    data.ss << " -------- |";
                }
                data.tableHeader = true;
            }
            data.ss << "\n";
            data.eol = true;
            break;
        }
        case XmlTextParser::Node::Type::TABLE_CELL: {
            data.ss << " |";
            data.eol = false;
            break;
        }
        case XmlTextParser::Node::Type::ONLYFOR: {
            data.ss << ")";
            data.eol = false;
            break;
        }
        default: {
            break;
        }
    }
}

void Doxybook2::TextMarkdownPrinter::programlisting(std::stringstream& ss, const XmlTextParser::Node& node) const {
    switch (node.type) {
        case XmlTextParser::Node::Type::TEXT: {
            ss << node.data;
            break;
        }
        default: {
            break;
        }
    }

    for (const auto& child : node.children) {
        programlisting(ss, child);
    }

    switch (node.type) {
        case XmlTextParser::Node::Type::CODELINE: {
            ss << "\n";
            break;
        }
        case XmlTextParser::Node::Type::SP: {
            ss << " ";
            break;
        }
        default: {
            break;
        }
    }
}
