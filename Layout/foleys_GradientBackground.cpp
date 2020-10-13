/*
 ==============================================================================
    Copyright (c) 2019-2020 Foleys Finest Audio Ltd. - Daniel Walz
    All rights reserved.

    License for non-commercial projects:

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    License for commercial products:

    To sell commercial products containing this module, you are required to buy a
    License from https://foleysfinest.com/developer/pluginguimagic/

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.
 ==============================================================================
 */


namespace foleys
{

void GradientBackground::drawGradient (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::Path& shape)
{
    if (isEmpty())
        return;

    juce::Graphics::ScopedSaveState state (g);

    auto vec = juce::Point<float>().getPointOnCircumference (bounds.getHeight() / 2, angle);

    auto p1 = bounds.getCentre() + vec;
    auto p2 = bounds.getCentre() - vec;

    if (gradient.point1 != p1 || gradient.point2 != p2)
    {
        gradient.clearColours();
        gradient.point1 = p1;
        gradient.point2 = p2;
        gradient.isRadial = type == radial;
        for (auto& c : colours)
            gradient.addColour (c.first, c.second);
    }

    g.setFillType (gradient);

    g.fillPath (shape);
}

void GradientBackground::setup (juce::String text, const Stylesheet& stylesheet)
{
    clear();

    if (text.isEmpty())
        return;

    if (text.startsWith ("linear"))
        type = linear;
    else if (text.startsWith ("radial"))
        type = radial;

    text = text.fromFirstOccurrenceOf ("(", false, false).upToLastOccurrenceOf (")", false, false);

    auto values = juce::StringArray::fromTokens (text, ",;", "\"");
    if (values.size() < 2)
        return;

    if (type == linear)
    {
        angle = juce::degreesToRadians (values [0].getFloatValue());
        values.remove (0);
    }

    auto step = 1.0f / (values.size() - 1.0f);
    auto stop = 0.0f;
    for (const auto& v : values)
    {
        colours [stop] = stylesheet.getColour (v);
        stop += step;
    }
}

void GradientBackground::clear()
{
    type = none;
    colours.clear();
}

bool GradientBackground::isEmpty() const
{
    return type == none || colours.size() < 2;
}

} // namespace foleys
