#pragma once

#include <cstdint>
#include <initializer_list>
#include <queue>
#include <span>
#include <stack>
#include <variant>
#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "util.h"

struct GridComponent {
    GridComponent(uint32_t componentIndex, ImRect baseRect)
        : m_index{componentIndex},
        m_baseRect{baseRect},
        m_size{baseRect.GetSize()},
        m_center{baseRect.GetCenter()} {}

    const ImRect& Rect() const { return m_baseRect; }

    ImVec2 Size() const { return m_size; }

    const ImVec2& Center() const { return m_center; }

    uint32_t GetIndex() const { return m_index; }

    GridComponent Scale(ImVec2 scale) const {
        ImRect rect = Rect();
        rect.Min *= scale;
        rect.Max *= scale;
        return GridComponent(m_index, rect);
    }

    GridComponent Pad(float top, float right, float bottom, float left) const {
        ImRect rect = Rect();
        rect.Min += ImVec2{left, top};
        rect.Max -= ImVec2{right, bottom};
        return GridComponent(m_index, rect);
    }

private:
    uint32_t m_index{};
    ImRect m_baseRect;
    ImVec2 m_size;
    ImVec2 m_center;
};

struct GridLayout {
    GridLayout(std::span<GridComponent> components)
    : m_baseComponents(components.begin(), components.end()) {}

    const GridComponent& GetComponent(size_t index) const {
        return m_baseComponents.at(index);
    }

    size_t NumComponnets() const { return m_baseComponents.size(); }

    GridLayout Scale(float scale) {
        return _Map([&](const GridComponent& comp) {
            return comp.Scale(ImVec2{scale, scale});
        });
    }

    GridLayout Scale(ImVec2 scale) {
        return _Map(
            [&](const GridComponent& comp) { return comp.Scale(scale); });
    }

    GridLayout Pad(float value) { return Pad(value, value, value, value); }

    GridLayout Pad(float top, float right, float bottom, float left) {
        return _Map([&](const GridComponent& comp) {
            return comp.Pad(top, right, bottom, left);
        });
    }

private:
    GridLayout _Map(auto componentFunc) {
        std::vector<GridComponent> mapped;
        mapped.reserve(m_baseComponents.size());
        for (auto& comp : m_baseComponents) {
            mapped.push_back(componentFunc(comp));
        }
        return GridLayout(mapped);
    }

    std::vector<GridComponent> m_baseComponents{};
};

inline ImVec4 operator*(const ImVec4& v, float t) {
    return ImVec4{v.x * t, v.y * t, v.z * t, v.w * t};
}

inline ImVec4 lerp(const ImVec4& lhs, const ImVec4& rhs, float t) {
    return lhs * t + rhs * (1.0f - t);
}

class GridLayoutBuilder {
    /// @brief Intermediate grid node with sub elements
    struct TreeNode {
        TreeNode() = default;
        TreeNode(ImVec2 origin, ImVec2 size) : origin{origin}, size{size} {}
        ImVec2 origin;
        ImVec2 size;
        bool isFinal = true;
        uint32_t* indexPtr = nullptr;
        std::vector<TreeNode> subElements;
    };

    struct SplitOp {
        int count;
        std::vector<float> weights;
        bool horizontal;
    };

    struct GetIndexOp {
        uint32_t* ptr;
        // If true, get index of childIdx; if false, get index of current node
        bool child;
        uint32_t childIdx;
    };

    struct PushOp {
        int index;
    };

    struct PopOp {};

    using Op = std::variant<SplitOp, PushOp, PopOp, GetIndexOp>;

    std::vector<float> MakeUniformWeights(int n) {
        return std::vector<float>(n, 1.0f);
    }

    std::deque<Op> m_ops;
    ImVec2 m_shape;

public:
    GridLayoutBuilder(ImVec2 shape) : m_shape{shape} {}
    GridLayoutBuilder() : m_shape{ImVec2{1.0f, 1.0f}} {}

    GridLayoutBuilder& AddRows(int numRows) {
        m_ops.push_back(SplitOp{.count = numRows,
            .weights = MakeUniformWeights(numRows),
            .horizontal = false});
        return *this;
    }

    GridLayoutBuilder& AddRowsEx(int numRows,
                                 std::initializer_list<float> weights) {
        m_ops.push_back(SplitOp{
            .count = numRows,
            .weights = std::vector<float>(weights.begin(), weights.end()),
            .horizontal = false});
        return *this;
    }

    GridLayoutBuilder& AddColumns(int numColumns) {
        m_ops.push_back(SplitOp{.count = numColumns,
            .weights = MakeUniformWeights(numColumns),
            .horizontal = true});
        return *this;
    }

    GridLayoutBuilder& AddColumnsEx(int numColumns,
                                    std::initializer_list<float> weights) {
        m_ops.push_back(SplitOp{
            .count = numColumns,
            .weights = std::vector<float>(weights.begin(), weights.end()),
            .horizontal = true});
        return *this;
    }

    GridLayoutBuilder& GetIndex(uint32_t* idxPtr, int childIdx = -1) {
        if (childIdx < 0) {
            m_ops.push_back(
                GetIndexOp{.ptr = idxPtr, .child = false, .childIdx = 0});
        } else {
            m_ops.push_back(
                GetIndexOp{.ptr = idxPtr,
                    .child = true,
                    .childIdx = static_cast<uint32_t>(childIdx)});
        }
        return *this;
    }

    GridLayoutBuilder& GetIndexN(std::initializer_list<uint32_t*> idxPtrs) {
        uint32_t childIdx{0U};
        for (auto ptr : idxPtrs) {
            m_ops.push_back(GetIndexOp{.ptr = ptr, .child = true, .childIdx = childIdx});
            ++childIdx;
        }
        return *this;
    }

    GridLayoutBuilder& GetIndexXY(uint32_t x, uint32_t y, uint32_t* idx) {
        Push(y);
        GetIndex(idx, x);
        Pop();
        return *this;
    };

    GridLayoutBuilder& Push(int index) {
        m_ops.push_back(PushOp{index});
        return *this;
    }

    GridLayoutBuilder& Pop() {
        m_ops.push_back(PopOp{});
        return *this;
    }

    GridLayoutBuilder& MakeRectGrid(int n, int m) {
        AddRows(m);
        for (int j = 0; j < m; ++j) {
            Push(j).AddColumns(n).Pop();
        }

        return *this;
    };

    GridLayout Build() {
        TreeNode root = TreeNode(ImVec2{0.0f, 0.0f}, ImVec2{1.0f, 1.0f});
        std::stack<TreeNode*> path;
        TreeNode* current = &root;
        path.push(&root);

        while (!m_ops.empty()) {
            Op frontOp = m_ops.front();
            m_ops.pop_front();

            auto visitor = Overload{
                [&path](SplitOp op) {
                    TreeNode* current = path.top();
                    if (!current->isFinal) {
                        throw std::runtime_error(
                            "Layout element already has subcomponents.");
                    }

                    current->isFinal = false;
                    float totalWeight = static_cast<float>(std::accumulate(
                        op.weights.begin(), op.weights.end(), 0.0f));
                    ImVec2 origin = current->origin;
                    current->subElements.resize(op.count);
                    for (int i = 0; i < op.count; ++i) {
                        if (op.horizontal) {
                            float newWidth = current->size.x *
                                op.weights.at(i) / totalWeight;
                            float newHeight = current->size.y;
                            current->subElements.at(i) =
                                TreeNode(origin, ImVec2(newWidth, newHeight));
                            origin.x += newWidth;
                        } else {
                            float newWidth = current->size.x;
                            float newHeight = current->size.y *
                                op.weights.at(i) / totalWeight;
                            current->subElements.at(i) =
                                TreeNode(origin, ImVec2(newWidth, newHeight));
                            origin.y += newHeight;
                        }
                    }
                },
                [&path](PushOp op) {
                    TreeNode* current = path.top();
                    path.push(&current->subElements.at(op.index));
                },
                [&path](PopOp op) { path.pop(); },
                [&path](GetIndexOp op) {
                    TreeNode* current = path.top();
                    if (op.child) {
                        current->subElements.at(op.childIdx).indexPtr = op.ptr;
                    } else {
                        current->indexPtr = op.ptr;
                    }
                },
            };

            std::visit(visitor, frontOp);
        }

        // DFS traversal to generate final list
        std::vector<GridComponent> result{};
        std::stack<TreeNode*> traversal;
        traversal.push(&root);

        // DFS first pass: just count the objects
        uint32_t numElements = 0;
        while (!traversal.empty()) {
            TreeNode* frontElem = traversal.top();
            traversal.pop();
            if (frontElem->isFinal) {
                ++numElements;
            } else {
                for (auto& subElem : frontElem->subElements) {
                    traversal.push(&subElem);
                }
            }
        }

        // DFS second pass: fill the result and set indices
        traversal = std::stack<TreeNode*>();
        traversal.push(&root);

        while (!traversal.empty()) {
            TreeNode* frontElem = traversal.top();
            traversal.pop();
            if (frontElem->isFinal) {
                uint32_t componentIdx = numElements - result.size() - 1;
                if (frontElem->indexPtr) {
                    // The index should be inverted because we do reverse later
                    *(frontElem->indexPtr) = componentIdx;
                }
                result.push_back(GridComponent(componentIdx, 
                                               ImRect{frontElem->origin,
                                               frontElem->origin + frontElem->size}));
            } else {
                for (auto& subElem : frontElem->subElements) {
                    traversal.push(&subElem);
                }
            }
        }

        std::reverse(result.begin(), result.end());
        auto layout = GridLayout(result);
        return layout.Scale(m_shape);
    }
};
